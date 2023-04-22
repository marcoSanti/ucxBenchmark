#include "ucxHelper.h"


#define __SERVER_PORT__ 12345


///request handler functions
static void request_init(void *request)
{
	struct ucx_context *ctx = (struct ucx_context *)request;
	ctx->completed = 0;
} 

static void send_handler(void *request, ucs_status_t status, void *ctx)
{
    struct ucx_context *context = (struct ucx_context *) ctx;
    context->completed = 1;
	printf("[INFO] Send complete @ send_handler()\n");
}

static void recv_handler(void *request, ucs_status_t status, ucp_tag_recv_info_t *info)
{
    struct ucx_context *context = (struct ucx_context *) request;
    context->completed = 1;
   	printf("[INFO] Recive complete @ send_handler()\n");
}


int main(int argc, char *argv[]){

    peerAddrInfo *peerInfo;
	ucp_ep_h endpoint;

	ucp_context_h context = bootstrapUcx(
		request_init,
		UCP_PARAM_FIELD_FEATURES | UCP_PARAM_FIELD_REQUEST_SIZE | UCP_PARAM_FIELD_REQUEST_INIT,
		UCP_FEATURE_TAG);
    
    printf("[info] Context created\n");

	ucp_worker_h worker = getUcxWorker(
		context,
		UCP_WORKER_PARAM_FIELD_THREAD_MODE,
		UCS_THREAD_MODE_SINGLE);

    printf("[info] Worker created\n");
	
    if (argc == 1) //server
	{
		peerInfo = server_handshake(__SERVER_PORT__, worker);
        printf("[INFO] Server: obtained client informations\n");
		endpoint = getEndpoint(worker, peerInfo, UCP_EP_PARAM_FIELD_REMOTE_ADDRESS);
		printf("[INFO] Server: created client endpoint\n");
        printf("[INFO] Server is ready to comunicate\n");
 	}
	else //client
	{
		peerInfo = client_handshake(argv[1], __SERVER_PORT__, worker);
        printf("[INFO] Client: obtained server informations\n");
		endpoint = getEndpoint(worker, peerInfo, UCP_EP_PARAM_FIELD_REMOTE_ADDRESS);
		printf("[INFO] Client: created server informations\n");
		printf("[INFO] client is ready to comunicate\n");
	}

}