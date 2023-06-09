#include "ucxHelper.h"
#include <sys/time.h>

/**
 * This file is an alternate version in which a two way handshakle is not required. 
 * In this case, there are two endpoints:
 * 	DEVICE (-d): the rereciving part of the communication
 * 	HOST (-h): the sending part of the communication
 * 
 * To use the woftware, two process must be launched as follows (launch first the host side process):
 * 
 *	(DEVICE) ./hostdevice -d <host ip address>
	(HOST) ./hostdevice -h  <send_iteration> <transfer_window_size>
 * 
 */

#define __DEVICE_PORT__ 12345
#define __TEMP_FILE_NAME__ "temp.dat"

extern int errno;

FILE *generate_file(double bytes)
{
	srand(getpid());

	char *buffer = malloc(bytes);

	int out = open(__TEMP_FILE_NAME__, O_CREAT | O_WRONLY | O_TRUNC, 0666);

	if (out < 1)
	{
		printf("%s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

	int rand_src = open("/dev/random", O_RDONLY);

	read(rand_src, buffer, bytes);
	if (write(out, buffer, bytes) < 1)
	{
		printf("%s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

	close(rand_src);
	close(out);

	FILE *fp;
	if ((fp = fopen(__TEMP_FILE_NAME__, "r")) == NULL)
	{

		printf("%s\n", strerror(errno));
	}
	return fp;
}

int main(int argc, char *argv[])
{

	struct timeval start, end;

	ucp_tag_t SETUP_TAG = 0;
	ucp_tag_t DATA_SEND_TAG = 1;
	ucp_tag_t TAG_MASK_BITS = 0;
	// ucp_tag_t TAG_MASK_BITS 	= 0xFFFFFFFFFFFFFFFF;

	peerAddrInfo *peerInfo;
	ucp_ep_h endpoint;

	ucs_status_ptr_t request_status;

	req_t requestContext;
	requestContext.completed = 0;

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

	if (strcmp(argv[1], "-d") == 0) // Remote reciving device
	{


		device_handshake(argv[2], __DEVICE_PORT__, worker);
	
		printf("[INFO] DEVICE is ready to comunicate\n");

		ucp_request_param_t *recvParam = getTagSendReciveParametersSingle(
			UCP_OP_ATTR_FIELD_CALLBACK | UCP_OP_ATTR_FIELD_DATATYPE | UCP_OP_ATTR_FIELD_USER_DATA,
			&requestContext,
			default_recv_handler);

		int iterations = 0, winSize = 0;


		// recive of bufferSize
		waitForMessageWithTag(worker, SETUP_TAG, TAG_MASK_BITS, NULL);
		request_status = ucp_tag_recv_nbx(worker, &iterations, sizeof(iterations), SETUP_TAG, TAG_MASK_BITS, recvParam);
		ucpWait(worker, request_status, &requestContext);
		// recive of winsize
		waitForMessageWithTag(worker, SETUP_TAG, TAG_MASK_BITS, NULL);
		request_status = ucp_tag_recv_nbx(worker, &winSize, sizeof(winSize), SETUP_TAG, TAG_MASK_BITS, recvParam);

		long int transferSize = iterations * winSize;
		printf("[INFO] Recived config from HOST: \n\tsendIterations: %d,\n\twinSize: %d,\n\ttotal transfer size:%ld\n", iterations, winSize, transferSize);

		gettimeofday(&start, NULL);
		for (int i = 0; i < iterations; i++)
		{
			// probe for last message incoming
			waitForMessageWithTag(worker, DATA_SEND_TAG, TAG_MASK_BITS, NULL);
			request_status = ucp_tag_recv_nbx(worker, &winSize, sizeof(winSize), DATA_SEND_TAG, TAG_MASK_BITS, recvParam);
			ucpWait(worker, request_status, &requestContext);
		}
		gettimeofday(&end, NULL);
		double elapsedTime = (end.tv_sec + 0.000001 * end.tv_usec) - (start.tv_sec + 0.000001 * start.tv_usec);
		double bandwidth = transferSize / elapsedTime;

		printf("Completed!\n[RESULT] Time elapsed to recive %ld bytes: \n\t%lf seconds.\n\tBandwidth= %.2lf MBytes/s (%.2lf Mbit/s)\n", transferSize, elapsedTime, bandwidth / (1024 * 1024), 8*bandwidth/1e6);

		
	}
	else if ((strcmp(argv[1], "-h") == 0) && argc == 4)
	{

		peerInfo = host_handshake(__DEVICE_PORT__);
		printf("[INFO] HOST: obtained DEVICE informations\n");

		
		endpoint = getEndpoint(worker, peerInfo, UCP_EP_PARAM_FIELD_REMOTE_ADDRESS);
		printf("[INFO] HOST ready to comunicate\n");

		ucp_request_param_t *sendParam = getTagSendReciveParametersSingle(
			UCP_OP_ATTR_FIELD_CALLBACK | UCP_OP_ATTR_FIELD_DATATYPE | UCP_OP_ATTR_FIELD_USER_DATA,
			&requestContext,
			default_send_handler);

		int iterations = atoi(argv[2]);
		int winsize = atoi(argv[3]);

		long int transferSize = iterations * winsize;

		FILE *dataSample = generate_file(transferSize);

		// first i send to the server the bufferSize
		request_status = ucp_tag_send_nbx(endpoint, &iterations, sizeof(iterations), SETUP_TAG, sendParam);
		ucpWait(worker, request_status, &requestContext);

		printf("[INFO] Sent configs to DEVICE:\tsendIterations->%d\n", iterations);

		// now i sent the windowsSize
		request_status = ucp_tag_send_nbx(endpoint, &winsize, sizeof(winsize), SETUP_TAG, sendParam);
		ucpWait(worker, request_status, &requestContext);
		printf("[INFO] Sent configs to DEVICE:\twindow size->%d\n", winsize);

		printf("[INFO] Total transfer size: %d\n", winsize * iterations);

		printf("[INFO] Setup complete... starting benchmark...\n");
		// now i begin the benchmark

		unsigned long int filePosition = 0;
		char *buffer = malloc(winsize);

		gettimeofday(&start, NULL);

		for (int i = 0; i < iterations; i++)
		{ // skipping last iteration to change tag to tell server to stop reciving
			fseek(dataSample, filePosition, SEEK_SET);
			fread(buffer, winsize, 1, dataSample);

			request_status = ucp_tag_send_nbx(endpoint, buffer, winsize, DATA_SEND_TAG, sendParam);
			ucpWait(worker, request_status, &requestContext);

			filePosition += winsize;
		}

		gettimeofday(&end, NULL);
	

		double elapsedTime = (end.tv_sec + 0.000001 * end.tv_usec) - (start.tv_sec + 0.000001 * start.tv_usec);
		double bandwidth = transferSize / elapsedTime;

		printf("Completed!\n[RESULT] Time elapsed to send %ld bytes: \n\t%lf seconds.\n\tBandwidth= %.2lf MBytes/s (%.2lf Mbit/s)\n", transferSize, elapsedTime, bandwidth / (1024 * 1024), 8*bandwidth/1e6);
		
		remove(__TEMP_FILE_NAME__);
	}
	else
	{
		printf("\n\nUsage ./benchmark [-s | -c <server_ip> <sendIterations> <window_size>]\n");
	}

	return 0;
}