#include "ucxHelper.h"

#define _TEST_STR_SRV_ "TEST MSG FROM SERVER\0"
#define _TEST_STR_CLI_ "TEST MSG FROM CLIENT\0"

/**
 * static functions for socket testing
 *
 */

void sendTestMessage(int sockd, int isServer)
{
	if(isServer){
		char *buffer = malloc(strlen(_TEST_STR_SRV_));
		memset(buffer, 0, strlen(_TEST_STR_SRV_));
		memcpy(buffer, _TEST_STR_SRV_, strlen(_TEST_STR_SRV_));

		send(sockd, buffer, strlen(_TEST_STR_SRV_), 0);
		free(buffer);
	}else{
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
 * @return ucp_config_t*
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
	ucp_params.request_size = sizeof(struct ucx_context); // The size of a reserved space in a non-blocking requests. Typically applications use this space for caching own structures in order to avoid costly memory allocations, pointer dereferences, and cache misses.
	ucp_params.request_init = request_init;

	status = ucp_config_read(NULL, NULL, &ucp_config);
	CHKERR_ACTION(status!=UCS_OK, "[ERROR]: Unable to read configuration @ bootstrapUcx()\n", exit(EXIT_FAILURE));

	// inizializzo il contesto ucp, ottenendo in output il contestop
	status = ucp_init(&ucp_params, ucp_config, &ucp_context);
	CHKERR_ACTION(status!=UCS_OK, "[ERROR]: Unable to read init UCX @ bootstrapUcx()\n", exit(EXIT_FAILURE));

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
	CHKERR_ACTION(status!=UCS_OK, "[ERROR]: Unable to create worker()\n", exit(EXIT_FAILURE));

	return ucp_worker;
}

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
	CHKERR_ACTION(ret!=0, "[ERROR]: unable to setsockopt @server_handshake()\n", exit(EXIT_FAILURE));

	inaddr.sin_family = AF_INET;
	inaddr.sin_port = htons(server_port);
	inaddr.sin_addr.s_addr = INADDR_ANY;

	memset(inaddr.sin_zero, 0, sizeof(inaddr.sin_zero));

	ret = bind(lsock, (struct sockaddr *)&inaddr, sizeof(inaddr));
	CHKERR_ACTION(ret!=0, "[ERROR]: unable to bind socket @server_handshake()\n", exit(EXIT_FAILURE));

	ret = listen(lsock, 0);
	CHKERR_ACTION(ret!=0, "[ERROR]: unable to listen on socket @server_handshake()\n", exit(EXIT_FAILURE));

	fprintf(stdout, "[INFO] Server is waiting for connection...\n");
	/* Accept next connection */
	dsock = accept(lsock, NULL, NULL);
	fprintf(stdout, "[INFO] Server has accepted a connection\n");

	close(lsock);

	ret = ucp_worker_get_address(worker, &local_addr, &addr_len);
	CHKERR_ACTION(ret!=UCS_OK, "[ERROR]: unable to get worker local addresss @server_handshake()\n", exit(EXIT_FAILURE));

	ret = send(dsock, &addr_len, sizeof(addr_len), 0);
	CHKERR_ACTION(ret==-1, "[ERROR]: unable to send address length @ server_handshake()\n", exit(EXIT_FAILURE));
	ret = send(dsock, local_addr, addr_len, 0);
	CHKERR_ACTION(ret==-1, "[ERROR]: unable to send address @ server_handshake()\n", exit(EXIT_FAILURE));

	ret = recv(dsock, &(peerInfo->addr_len), sizeof(peerInfo->addr_len), MSG_WAITALL);
	CHKERR_ACTION(ret==-1, "[ERROR]: unable to recive peer address length @ server_handshake()\n", exit(EXIT_FAILURE));

	peerInfo->peer_addr = malloc(peerInfo->addr_len);
	ret = recv(dsock, peerInfo->peer_addr , peerInfo->addr_len, MSG_WAITALL);
	CHKERR_ACTION(ret==-1, "[ERROR]: unable to recive peer address @ server_handshake()\n", exit(EXIT_FAILURE));

#ifdef __DEBUG__
	sendTestMessage(dsock, 1);
	reciveTestMessage(dsock);
#endif

	free(local_addr);

	return peerInfo;
}

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
	CHKERR_ACTION(ret!=0, "[ERROR]: unable to connect to socket @clien_handshake()\n", exit(EXIT_FAILURE));

	ret = ucp_worker_get_address(worker, &local_addr, &addr_len);
	CHKERR_ACTION(ret!=UCS_OK, "[ERROR]: unable to get worker local addresss @client_handshake()\n", exit(EXIT_FAILURE));


	ret = recv(connfd, &(peerInfo->addr_len), sizeof(peerInfo->addr_len), MSG_WAITALL);
	CHKERR_ACTION(ret==-1, "[ERROR]: unable to recive peer addresss length @client_handshake()\n", exit(EXIT_FAILURE));
	peerInfo->peer_addr = malloc(peerInfo->addr_len);
	ret = recv(connfd, peerInfo->peer_addr, peerInfo->addr_len, MSG_WAITALL);
	CHKERR_ACTION(ret==-1, "[ERROR]: unable to recive peer addresss @client_handshake()\n", exit(EXIT_FAILURE));

	ret = send(connfd, &addr_len, sizeof(addr_len), 0);
	CHKERR_ACTION(ret==-1, "[ERROR]: unable to send local addresss length @client_handshake()\n", exit(EXIT_FAILURE));
	ret = send(connfd, local_addr, addr_len, 0);
	CHKERR_ACTION(ret==-1, "[ERROR]: unable to send local addresss @client_handshake()\n", exit(EXIT_FAILURE));

#ifdef __DEBUG__
	reciveTestMessage(connfd);
	sendTestMessage(connfd, 0);
#endif

	free(local_addr);

	return peerInfo;
}

ucp_ep_h getEndpoint(ucp_worker_h worker, peerAddrInfo *peer, uint64_t ep_field_mask)
{
	ucp_ep_h endpoint;
	ucp_ep_params_t ep_params;
	ucs_status_t status;

	ep_params.field_mask = ep_field_mask;
	ep_params.address = peer->peer_addr;

	status = ucp_ep_create(worker, &ep_params, &endpoint);
	CHKERR_ACTION(status!=UCS_OK, "[ERROR]: unable to create endpoint @ getEndpoint()\n", exit(EXIT_FAILURE));

	return endpoint;
}
