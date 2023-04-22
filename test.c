#define __DEBUG__

#include "ucxHelper.h"

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
	ucp_context_print_info(context, stdout);

	ucp_worker_h worker = getUcxWorker(
		context,
		UCP_WORKER_PARAM_FIELD_THREAD_MODE,
		UCS_THREAD_MODE_SINGLE);

	ucp_worker_print_info(worker, stdout);

	if (argc == 1)
	{
		peerInfo = server_handshake(12345, worker);
		endpoint = getEndpoint(worker, peerInfo, UCP_EP_PARAM_FIELD_REMOTE_ADDRESS);
		ucp_ep_print_info(endpoint, stdout);

		ucp_tag_send_nbr(endpoint,"This is a blocking send test\0",strlen("This is a blocking send test\0"),UCP_DATATYPE_CONTIG,1,send_handler);		
	}
	else
	{ //client
		peerInfo = client_handshake(argv[1], 12345, worker);
		endpoint = getEndpoint(worker, peerInfo, UCP_EP_PARAM_FIELD_REMOTE_ADDRESS);
		ucp_ep_print_info(endpoint, stdout);
		char buffer[strlen("This is a blocking send test\0")+1];
		ucp_tag_recv_nbr(worker, &buffer, strlen("This is a blocking send test\0"), UCP_DATATYPE_CONTIG, 1,1,recv_handler);
		printf("%s\n", buffer);

	}
}