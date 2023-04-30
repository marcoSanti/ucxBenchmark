#include "ucxHelper.h"
#include <sys/time.h>


#define __SERVER_PORT__ 12345
#define __TEMP_FILE_NAME__ "temp.dat"


extern int errno;


FILE* generate_file(double bytes){
    srand(getpid());

    char* buffer = malloc(bytes);

    int out = open(__TEMP_FILE_NAME__, O_CREAT | O_WRONLY | O_TRUNC, 0666);

    if(out < 1){
        printf("%s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    int rand_src = open("/dev/random",O_RDONLY);

    read(rand_src, buffer, bytes);
    if( write(out, buffer, bytes) < 1){
        printf("%s\n",  strerror(errno));
        exit(EXIT_FAILURE);
    }
    
    close(rand_src);
    close(out);

    FILE* fp;
    if((fp =fopen(__TEMP_FILE_NAME__, "r")) == NULL ){
     
        printf("%s\n", strerror(errno));
    }
    return fp;
}




int main(int argc, char *argv[]){

	ucp_tag_t SETUP_TAG 		= 0;
	ucp_tag_t DATA_SEND_TAG 	= 1;
	ucp_tag_t DATA_CONCLUDE_TAG = 2;
	ucp_tag_t TAG_MASK_BITS 	= 0;
	//ucp_tag_t TAG_MASK_BITS 	= 0xFFFFFFFFFFFFFFFF;

    peerAddrInfo *peerInfo;
	ucp_ep_h endpoint;

	ucs_status_ptr_t request_status;

	req_t requestContext;
	requestContext.completed=0;
		

	ucp_context_h context = bootstrapUcx(
		default_request_init,
		UCP_PARAM_FIELD_FEATURES | UCP_PARAM_FIELD_REQUEST_SIZE | UCP_PARAM_FIELD_REQUEST_INIT,
		UCP_FEATURE_TAG);
    
    printf("[info] Context created\n");

	ucp_worker_h worker = getUcxWorker(
		context,
		UCP_WORKER_PARAM_FIELD_THREAD_MODE,
		UCS_THREAD_MODE_SINGLE);

    printf("[info] Worker created\n");

	
    if (strcmp(argv[1], "-s") == 0) //server
	{
		peerInfo = server_handshake(__SERVER_PORT__, worker);
        printf("[INFO] Server: obtained client informations\n");
		endpoint = getEndpoint(worker, peerInfo, UCP_EP_PARAM_FIELD_REMOTE_ADDRESS);
		printf("[INFO] Server: created client endpoint\n");
        printf("[INFO] Server is ready to comunicate\n");

		ucp_request_param_t* recvParam = getTagSendReciveParametersSingle(
			UCP_OP_ATTR_FIELD_CALLBACK | UCP_OP_ATTR_FIELD_DATATYPE | UCP_OP_ATTR_FIELD_USER_DATA,
			&requestContext,
			default_recv_handler );
		
		int iterations=0, winSize=0;

		//recive of bufferSize
		request_status = ucp_tag_recv_nbx(worker, &iterations, sizeof(iterations), SETUP_TAG, TAG_MASK_BITS, recvParam);
		ucpWait(worker, &request_status, &requestContext);

		//recive of winsize
		request_status = ucp_tag_recv_nbx(worker, &winSize, sizeof(winSize), SETUP_TAG, TAG_MASK_BITS, recvParam);
		ucpWait(worker, &request_status, &requestContext);

		long int transferSize = iterations * winSize;
		printf("[INFO] Recived config from client: \n\tsendIterations: %d,\n\twinSize: %d,\n\ttotal transfer size:%ld\n", iterations , winSize, transferSize);

		ucp_tag_message_h msg_tag;
		ucp_tag_recv_info_t info_tag;

		for(int i = 0;; i++){

			//probe for last message incoming
			msg_tag = ucp_tag_probe_nb(worker, DATA_CONCLUDE_TAG, TAG_MASK_BITS, 1, &info_tag);
			if (msg_tag != NULL) {
				// last message incoming, so recive and conclude 
				request_status = ucp_tag_recv_nbx(worker, &winSize, sizeof(winSize), DATA_CONCLUDE_TAG, TAG_MASK_BITS, recvParam);
				ucpWait(worker, &request_status, &requestContext);
            	break;
        	}

			request_status = ucp_tag_recv_nbx(worker, &winSize, sizeof(winSize), DATA_SEND_TAG, TAG_MASK_BITS, recvParam);
			if(request_status != NULL)
				ucpWait(worker, &request_status, &requestContext);		
			printf("[DATA] recived data chunk from client\n");
			
		}



 	}
	else if((strcmp(argv[1], "-c")==0) && argc == 5 )
	{
		peerInfo = client_handshake(argv[2], __SERVER_PORT__, worker);
        printf("[INFO] Client: obtained server informations\n");
		endpoint = getEndpoint(worker, peerInfo, UCP_EP_PARAM_FIELD_REMOTE_ADDRESS);
		printf("[INFO] Client: created server informations\n");
		printf("[INFO] client is ready to comunicate\n");

		ucp_request_param_t* sendParam = getTagSendReciveParametersSingle(
			UCP_OP_ATTR_FIELD_CALLBACK | UCP_OP_ATTR_FIELD_DATATYPE | UCP_OP_ATTR_FIELD_USER_DATA,
			&requestContext,
			default_send_handler );

		int iterations = atoi(argv[3]);
		int winsize = atoi(argv[4]);

		long int transferSize = iterations * winsize;

		printf("[INFO] Sending configs to server: \n\tsendIterations: %d, \n\twinSize: %d\n\ttotal transfer size: %ld\n", iterations, winsize, transferSize);

		FILE* dataSample = generate_file(transferSize);
		

		//first i send to the server the bufferSize
		request_status = ucp_tag_send_nbx(endpoint, &iterations, sizeof(iterations), SETUP_TAG, sendParam);
		ucpWait(worker, &request_status, &requestContext);
		

		//now i sent the windowsSize
		request_status = ucp_tag_send_nbx(endpoint, &winsize, sizeof(winsize), SETUP_TAG, sendParam);
		ucpWait(worker, &request_status, &requestContext);

		printf("[INFO] Setup complete... starting benchmark...\n");
		//now i begin the benchmark

		unsigned long int filePosition = 0;
		char* buffer = malloc(winsize);
		struct timeval start, end;

		gettimeofday(&start, NULL);

		for(int i= 0; i<iterations-1; i++){ //skipping last iteration to change tag to tell server to stop reciving
			fseek(dataSample, filePosition, SEEK_SET);
			fread(buffer, winsize, 1, dataSample);
			
			request_status = ucp_tag_send_nbx(endpoint, buffer, winsize, DATA_SEND_TAG, sendParam);
			ucpWait(worker, &request_status, &requestContext);

			if(i % (iterations/10) == 0)
				printf("."); //print a dot evry 10%of the benchmark

			filePosition+=winsize;
		}

		fseek(dataSample, filePosition, SEEK_SET);
		fread(buffer, winsize, 1, dataSample);
		
		request_status = ucp_tag_send_nbx(endpoint, buffer, winsize, DATA_CONCLUDE_TAG, sendParam);
		ucpWait(worker, &request_status, &requestContext);

		gettimeofday(&end, NULL);

		double elapsedTime = (end.tv_sec + 0.000001 * end.tv_usec) - (start.tv_sec + 0.000001 * start.tv_usec);
		double bandwidth = transferSize / elapsedTime;

		printf("Completed!\n[RESULT] Time elapsed to transfer %ld bytes: \n\t%lf seconds.\n\tBandwidth= %.2lf MBytes / seconds\n", transferSize, elapsedTime, bandwidth/(1024*1024));

	}else{
		printf("\n\nUsage ./benchmark [-s | -c <server_ip> <sendIterations> <window_size>]\n");
	}

	free(request_status);
	return 0;
}