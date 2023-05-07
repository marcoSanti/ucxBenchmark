#include "ucxHelper.h"

#define _TEST_STR_SRV_ "TEST MSG FROM SERVER\0"
#define _TEST_STR_CLI_ "TEST MSG FROM CLIENT\0"

/**
 * static functions for socket testing
 *
 */

void sendTestMessage(int sockd, int isServer)
{
	if (isServer)
	{
		char *buffer = malloc(strlen(_TEST_STR_SRV_));
		memset(buffer, 0, strlen(_TEST_STR_SRV_));
		memcpy(buffer, _TEST_STR_SRV_, strlen(_TEST_STR_SRV_));

		send(sockd, buffer, strlen(_TEST_STR_SRV_), 0);
		free(buffer);
	}
	else
	{
		char *buffer = malloc(strlen(_TEST_STR_CLI_));
		memset(buffer, 0, strlen(_TEST_STR_CLI_));
		memcpy(buffer, _TEST_STR_CLI_, strlen(_TEST_STR_CLI_));

		send(sockd, buffer, strlen(_TEST_STR_CLI_), 0);
		free(buffer);
	}
}

void reciveTestMessage(int sockd)
{
	char *buffer = malloc(strlen(_TEST_STR_SRV_));
	memset(buffer, 0, strlen(_TEST_STR_SRV_));

	int len = recv(sockd, buffer, strlen(_TEST_STR_SRV_), 0);

	printf("Recived bytes: %d\nMsg: %s\n", len, buffer);
	free(buffer);
}

/**
 * @brief this fuinction is used to generate a ucx configuration and initialize ucx
 * 
 * @param request_init function pointer that will initialize the variable that will tell when a request has been served
 * @param ucpParamsFields bitmask for requested parameters for ucx
 * @param ucpFeature Type of communication requested
 * @return ucp_context_h 
 */
ucp_context_h bootstrapUcx(ucp_request_init_callback_t request_init, enum ucp_params_field ucpParamsFields, enum ucp_feature ucpFeature)
{
	ucp_config_t *ucp_config;
	ucp_params_t ucp_params;
	ucp_context_h ucp_context;
	ucs_status_t status = 0;

	memset(&ucp_params, 0, sizeof(ucp_params));

	ucp_params.field_mask = ucpParamsFields;
	ucp_params.features = ucpFeature;
	ucp_params.request_size = sizeof(req_t); // The size of a reserved space in a non-blocking requests. Typically applications use this space for caching own structures in order to avoid costly memory allocations, pointer dereferences, and cache misses.
	ucp_params.request_init = request_init;

	status = ucp_config_read(NULL, NULL, &ucp_config);
	CHKERR_ACTION(status != UCS_OK, "[ERROR]: Unable to read configuration @ bootstrapUcx()\n", exit(EXIT_FAILURE));

	// inizializzo il contesto ucp, ottenendo in output il contestop
	status = ucp_init(&ucp_params, ucp_config, &ucp_context);
	CHKERR_ACTION(status != UCS_OK, "[ERROR]: Unable to read init UCX @ bootstrapUcx()\n", exit(EXIT_FAILURE));

#ifdef __DEBUG__
	ucp_config_print(ucp_config, stdout, NULL, UCS_CONFIG_PRINT_CONFIG);
#endif
	ucp_config_release(ucp_config);

	return ucp_context;
}

/**
 * @brief Get the Ucx Worker object
 *
 * @param context
 * @param field_mask
 * @param thread_mode
 * @return ucp_worker_h
 */
ucp_worker_h getUcxWorker(ucp_context_h context, uint64_t field_mask, ucs_thread_mode_t thread_mode)
{
	ucp_worker_params_t worker_params;
	ucp_worker_h ucp_worker;
	ucs_status_t status = 0;

	memset(&worker_params, 0, sizeof(worker_params));

	worker_params.field_mask = field_mask;	 // BOH NO SO CHE SERVE> dice che serve per retrocompatibilita ma bog
	worker_params.thread_mode = thread_mode; // see

	status = ucp_worker_create(context, &worker_params, &ucp_worker);
	CHKERR_ACTION(status != UCS_OK, "[ERROR]: Unable to create worker()\n", exit(EXIT_FAILURE));

	return ucp_worker;
}



peerAddrInfo *host_handshake(uint16_t server_port)
{
	struct sockaddr_in inaddr;
	int lsock = -1;
	int dsock = -1;
	int optval = 1;
	int ret = 0;

	peerAddrInfo *peerInfo = (peerAddrInfo *)malloc(sizeof(peerAddrInfo));

	lsock = socket(AF_INET, SOCK_STREAM, 0);

	optval = 1;
	ret = setsockopt(lsock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
	CHKERR_ACTION(ret != 0, "[ERROR]: unable to setsockopt @host_handshake()\n", exit(EXIT_FAILURE));

	inaddr.sin_family = AF_INET;
	inaddr.sin_port = htons(server_port);
	inaddr.sin_addr.s_addr = INADDR_ANY;

	memset(inaddr.sin_zero, 0, sizeof(inaddr.sin_zero));

	ret = bind(lsock, (struct sockaddr *)&inaddr, sizeof(inaddr));
	CHKERR_ACTION(ret != 0, "[ERROR]: unable to bind socket @host_handshake()\n", exit(EXIT_FAILURE));

	ret = listen(lsock, 0);
	CHKERR_ACTION(ret != 0, "[ERROR]: unable to listen on socket @host_handshake()\n", exit(EXIT_FAILURE));

	fprintf(stdout, "[INFO] HOIST waiting for connection...\n");
	/* Accept next connection */
	dsock = accept(lsock, NULL, NULL);
	fprintf(stdout, "[INFO] HOST has accepted a connection\n");

	close(lsock);

	ret = recv(dsock, &(peerInfo->addr_len), sizeof(peerInfo->addr_len), MSG_WAITALL);
	CHKERR_ACTION(ret == -1, "[ERROR]: unable to recive peer address length @ host_handshake()\n", exit(EXIT_FAILURE));

	peerInfo->peer_addr = malloc(peerInfo->addr_len);
	ret = recv(dsock, peerInfo->peer_addr, peerInfo->addr_len, MSG_WAITALL);
	CHKERR_ACTION(ret == -1, "[ERROR]: unable to recive peer address @ host_handshake()\n", exit(EXIT_FAILURE));

#ifdef __DEBUG__
	sendTestMessage(dsock, 1);
	reciveTestMessage(dsock);
#endif


	return peerInfo;
}



void device_handshake(const char *server, uint16_t server_port, ucp_worker_h worker)
{
	ucp_address_t *local_addr;
	uint64_t addr_len = 0;
	struct sockaddr_in conn_addr;
	struct hostent *he;
	int connfd = -1;
	int ret = 0;

	connfd = socket(AF_INET, SOCK_STREAM, 0);

	he = gethostbyname(server);

	conn_addr.sin_family = he->h_addrtype;
	conn_addr.sin_port = htons(server_port);

	memcpy(&conn_addr.sin_addr, he->h_addr_list[0], he->h_length);
	memset(conn_addr.sin_zero, 0, sizeof(conn_addr.sin_zero));

	ret = connect(connfd, (struct sockaddr *)&conn_addr, sizeof(conn_addr));
	CHKERR_ACTION(ret != 0, "[ERROR]: unable to connect to socket @device_handshake()\n", exit(EXIT_FAILURE));

	ret = ucp_worker_get_address(worker, &local_addr, &addr_len);
	CHKERR_ACTION(ret != UCS_OK, "[ERROR]: unable to get worker local addresss @device_handshake()\n", exit(EXIT_FAILURE));

	ret = send(connfd, &addr_len, sizeof(addr_len), 0);
	CHKERR_ACTION(ret == -1, "[ERROR]: unable to send local addresss length @device_handshake()\n", exit(EXIT_FAILURE));
	ret = send(connfd, local_addr, addr_len, 0);
	CHKERR_ACTION(ret == -1, "[ERROR]: unable to send local addresss @device_handshake()\n", exit(EXIT_FAILURE));

#ifdef __DEBUG__
	reciveTestMessage(connfd);
	sendTestMessage(connfd, 0);
#endif

	free(local_addr);

}



/**
 * @brief This function performs a out of band handshake to share adresses between client and server. This function implements the server side version
 * 
 * @param server_port server port uint16_t format
 * @param worker the worker that will serve the request in the future. The functyion will share this woker address
 * @return peerAddrInfo* 
 */
peerAddrInfo *server_handshake(uint16_t server_port, ucp_worker_h worker)
{
	ucp_address_t *local_addr;
	uint64_t addr_len = 0;
	struct sockaddr_in inaddr;
	int lsock = -1;
	int dsock = -1;
	int optval = 1;
	int ret = 0;

	peerAddrInfo *peerInfo = (peerAddrInfo *)malloc(sizeof(peerAddrInfo));

	lsock = socket(AF_INET, SOCK_STREAM, 0);

	optval = 1;
	ret = setsockopt(lsock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
	CHKERR_ACTION(ret != 0, "[ERROR]: unable to setsockopt @server_handshake()\n", exit(EXIT_FAILURE));

	inaddr.sin_family = AF_INET;
	inaddr.sin_port = htons(server_port);
	inaddr.sin_addr.s_addr = INADDR_ANY;

	memset(inaddr.sin_zero, 0, sizeof(inaddr.sin_zero));

	ret = bind(lsock, (struct sockaddr *)&inaddr, sizeof(inaddr));
	CHKERR_ACTION(ret != 0, "[ERROR]: unable to bind socket @server_handshake()\n", exit(EXIT_FAILURE));

	ret = listen(lsock, 0);
	CHKERR_ACTION(ret != 0, "[ERROR]: unable to listen on socket @server_handshake()\n", exit(EXIT_FAILURE));

	fprintf(stdout, "[INFO] Server is waiting for connection...\n");
	/* Accept next connection */
	dsock = accept(lsock, NULL, NULL);
	fprintf(stdout, "[INFO] Server has accepted a connection\n");

	close(lsock);

	ret = ucp_worker_get_address(worker, &local_addr, &addr_len);
	CHKERR_ACTION(ret != UCS_OK, "[ERROR]: unable to get worker local addresss @server_handshake()\n", exit(EXIT_FAILURE));

	ret = send(dsock, &addr_len, sizeof(addr_len), 0);
	CHKERR_ACTION(ret == -1, "[ERROR]: unable to send address length @ server_handshake()\n", exit(EXIT_FAILURE));
	ret = send(dsock, local_addr, addr_len, 0);
	CHKERR_ACTION(ret == -1, "[ERROR]: unable to send address @ server_handshake()\n", exit(EXIT_FAILURE));

	ret = recv(dsock, &(peerInfo->addr_len), sizeof(peerInfo->addr_len), MSG_WAITALL);
	CHKERR_ACTION(ret == -1, "[ERROR]: unable to recive peer address length @ server_handshake()\n", exit(EXIT_FAILURE));

	peerInfo->peer_addr = malloc(peerInfo->addr_len);
	ret = recv(dsock, peerInfo->peer_addr, peerInfo->addr_len, MSG_WAITALL);
	CHKERR_ACTION(ret == -1, "[ERROR]: unable to recive peer address @ server_handshake()\n", exit(EXIT_FAILURE));

#ifdef __DEBUG__
	sendTestMessage(dsock, 1);
	reciveTestMessage(dsock);
#endif

	free(local_addr);

	return peerInfo;
}

/**
 * @brief This function performs a out of band handshake to share adresses between client and server. This function implements the client side version
 * 
 * @param server Server address in string format
 * @param server_port server port uint16_t format
 * @param worker the worker that will serve the request in the future. The functyion will share this woker address
 * @return peerAddrInfo* 
 */
peerAddrInfo *client_handshake(const char *server, uint16_t server_port, ucp_worker_h worker)
{
	ucp_address_t *local_addr;
	uint64_t addr_len = 0;
	struct sockaddr_in conn_addr;
	struct hostent *he;
	int connfd = -1;
	int ret = 0;

	peerAddrInfo *peerInfo = (peerAddrInfo *)malloc(sizeof(peerAddrInfo));

	connfd = socket(AF_INET, SOCK_STREAM, 0);

	he = gethostbyname(server);

	conn_addr.sin_family = he->h_addrtype;
	conn_addr.sin_port = htons(server_port);

	memcpy(&conn_addr.sin_addr, he->h_addr_list[0], he->h_length);
	memset(conn_addr.sin_zero, 0, sizeof(conn_addr.sin_zero));

	ret = connect(connfd, (struct sockaddr *)&conn_addr, sizeof(conn_addr));
	CHKERR_ACTION(ret != 0, "[ERROR]: unable to connect to socket @clien_handshake()\n", exit(EXIT_FAILURE));

	ret = ucp_worker_get_address(worker, &local_addr, &addr_len);
	CHKERR_ACTION(ret != UCS_OK, "[ERROR]: unable to get worker local addresss @client_handshake()\n", exit(EXIT_FAILURE));

	ret = recv(connfd, &(peerInfo->addr_len), sizeof(peerInfo->addr_len), MSG_WAITALL);
	CHKERR_ACTION(ret == -1, "[ERROR]: unable to recive peer addresss length @client_handshake()\n", exit(EXIT_FAILURE));
	peerInfo->peer_addr = malloc(peerInfo->addr_len);
	ret = recv(connfd, peerInfo->peer_addr, peerInfo->addr_len, MSG_WAITALL);
	CHKERR_ACTION(ret == -1, "[ERROR]: unable to recive peer addresss @client_handshake()\n", exit(EXIT_FAILURE));

	ret = send(connfd, &addr_len, sizeof(addr_len), 0);
	CHKERR_ACTION(ret == -1, "[ERROR]: unable to send local addresss length @client_handshake()\n", exit(EXIT_FAILURE));
	ret = send(connfd, local_addr, addr_len, 0);
	CHKERR_ACTION(ret == -1, "[ERROR]: unable to send local addresss @client_handshake()\n", exit(EXIT_FAILURE));

#ifdef __DEBUG__
	reciveTestMessage(connfd);
	sendTestMessage(connfd, 0);
#endif

	free(local_addr);

	return peerInfo;
}

/**
 * @brief Get the Endpoint object of ucx
 * 
 * @param worker the worker that will use the endpoint
 * @param peer the peer address
 * @param ep_field_mask Field mask for endpoint options
 * @return a ucx endpoint
 */
ucp_ep_h getEndpoint(ucp_worker_h worker, peerAddrInfo *peer, uint64_t ep_field_mask)
{
	ucp_ep_h endpoint;
	ucp_ep_params_t ep_params;
	ucs_status_t status;

	ep_params.field_mask = ep_field_mask;
	ep_params.address = peer->peer_addr;

	status = ucp_ep_create(worker, &ep_params, &endpoint);
	CHKERR_ACTION(status != UCS_OK, "[ERROR]: unable to create endpoint @ getEndpoint()\n", exit(EXIT_FAILURE));

	return endpoint;
}

/**
 * @brief This function implements a wait for ucp. it will spinlock untill the request is completed
 *
 * @param ucp_worker The worker that needs to wait for something
 * @param request_status The request on wich wait returned by ucp send or recive
 * @param ctx 
 * @return ucs_status_t
 * +
 */
ucs_status_t ucpWait(ucp_worker_h ucp_worker, ucs_status_ptr_t request_status, req_t* ctx)
{
	ucs_status_t status;

	/* if operation was completed immediately */
	if (request_status == NULL)
		return UCS_OK;
	

	if (UCS_PTR_IS_ERR(request_status)) 
		return UCS_PTR_STATUS(request_status);
	
	
	while( ctx->completed == 0 )
	{
		ucp_worker_progress(ucp_worker);
	}
	status = ucp_request_check_status(request_status);


	ucp_request_free(request_status);

	ctx->completed=0;

	return status;
}

/**
 * @brief This function implements a busy wait for non blocking recive ucx calls. It will loop and return as sson as a message is present. This function will not remove the incoming message from the queue!
 * 
 * @param worker Worker on wich to eait for an incoming message3
 * @param messageTag Message tag to wait for
 * @param messageTagMask  MEssage tag bitmask
 * @param incoming_msg pointer to a ucp_tag_recv_info_t data structure that will be filed with the incoming message info. if not interested in incoming message info, it can be set to NULL
 */
void waitForMessageWithTag(ucp_worker_h worker, ucp_tag_t messageTag, ucp_tag_t messageTagMask, ucp_tag_recv_info_t* incoming_msg){
	
	ucp_tag_recv_info_t* info_tag = malloc(sizeof(info_tag));

	while( ucp_tag_probe_nb(worker, messageTag, messageTagMask, 0, info_tag) == NULL) {
		ucp_worker_progress(worker);
	}

	if(incoming_msg == NULL){
		free(info_tag);
	}else{
		incoming_msg = info_tag;
	}


}


/**
 * @brief this function returns a parameters for a non blocking, tag based, send or recive, under the condition that a single buffer is sent. 
 * 
 * 
 * @param parameterMask Bitmask for the operation parameters
 * @param requestContext The context of the request. this is important as is the one containig the variable wich will tell when the request has been completed
 * @param callbackFunctionHandle The fuinction to execute once the request has completed. This functrion should update the parameter contained in request context
 * @return ucp_request_param_t* 
 */
ucp_request_param_t *getTagSendReciveParametersSingle(uint32_t parameterMask, req_t* requestContext, void* callbackFunctionHandle)
{
	ucp_request_param_t* param = malloc(sizeof(ucp_request_param_t));

	param->op_attr_mask = parameterMask;
	param->datatype = ucp_dt_make_contig(1);
	param->user_data = requestContext;
	param->cb.send = callbackFunctionHandle;

	return param;
}


/**
 * @brief This function implements a basic recive handler function 
 * 
 * @param request The request that has been completed
 * @param status Status of the request
 * @param info 
 * @param user_data the pointer to the structure that contains the variable that tells when a recive has been completed
 */

void default_recv_handler(void *request, ucs_status_t status, const ucp_tag_recv_info_t *info, void *user_data)
{
	if (status != UCS_OK)
	{
		printf("[Error] unable to recive message!. UCS error is: %s\n", ucs_status_string(status));
		return;
	}

	((req_t*)user_data)->completed = 1;

#ifdef __DEBUG__
	printf("[REQ_RECV_HANDLER] request recived completed.\n");
#endif
}



/**
 * @brief This functidefault_recv_handleron implements a basic recive handler function 
 * @param request The request that the send will handle
 * @param status the status of the request
 * @param user_data the pointer to the structure that contains the variable that tells when a recive has been completed
 */
void default_send_handler(void *request, ucs_status_t status, void *user_data)
{
	
	if (status != UCS_OK)
	{
		printf("[Error] unable to send message! Error code is: %s\n", ucs_status_string(status));
		return;
	}

	((req_t*)user_data)->completed = 1;

#ifdef __DEBUG__
	printf("[REQ_SEND_HANDLER] send completed: %d\n", ((req_t*)user_data)->completed);
#endif
}

/**
 * @brief This function implements a defalt function to initialize the ucx vcariable that thells when a request has been served
 * 
 * @param request the pointer to the structure that contains the variable
 */
void default_request_init(void *request)
{
	((req_t*)request)->completed = 0;
}
