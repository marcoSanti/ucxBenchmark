#define __DEBUG__

#include "ucxHelper.h"

static void request_init(void *request)
{
	struct ucx_context *ctx = (struct ucx_context *)request;
	ctx->completed = 0;
}

static void send_handler(void *request, ucs_status_t status, void *ctx)
{

	if (status != UCS_OK)
	{
		printf("[Error] unable to send message!");
		return;
	}
	struct ucx_context *context = (struct ucx_context *)ctx;
	context->completed = 1;
	printf("[INFO] Send complete @ send_handler(). context->completed=%d\n", context->completed);
}

static void recv_handler(void *request, ucs_status_t status, ucp_tag_recv_info_t *info, void *user_data)
{
	if (status != UCS_OK)
	{
		printf("[Error] unable to recive message!");
		return;
	}
	struct ucx_context *context = (struct ucx_context *)user_data;
	context->completed = 1;
	printf("[INFO] Recive complete @ recv_handler(). context->completed=%d\n", context->completed);
}

#define __TEST_STRING__ "CIao questo è un sample di invio di messaggio!\0"

static int request_finalize(ucp_worker_h ucp_worker, req_t *request, req_t *ctx, int is_server, ucp_dt_iov_t *iov)
{
	int ret = 0;
	ucs_status_t status;

	status = ucpWait(ucp_worker, request, ctx);
	if (status != UCS_OK)
	{
		fprintf(stderr, "unable to %s UCX message (%s)\n",
				is_server ? "receive" : "send", ucs_status_string(status));
		ret = -1;
		goto release_iov;
	}

release_iov:
	free(iov);
	return ret;
}

int main(int argc, char *argv[])
{

	req_t requestContext;

	requestContext.completed = 0;

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

	char *message = malloc(strlen(__TEST_STRING__));
	if (argc == 1)
	{
		peerInfo = server_handshake(12345, worker);
		endpoint = getEndpoint(worker, peerInfo, UCP_EP_PARAM_FIELD_REMOTE_ADDRESS);
		ucp_ep_print_info(endpoint, stdout);

		memcpy(message, __TEST_STRING__, strlen(__TEST_STRING__));

		// list of buffers to store the data. qua ne alloco uno solo
		// e init a zero la sua memoria
		ucp_dt_iov_t *iov = malloc(sizeof(ucp_dt_iov_t));
		memset(iov, 0, sizeof(*iov));

		ucp_request_param_t param;
		req_t *request;
		size_t message_length = strlen(__TEST_STRING__);
		

	
		param.op_attr_mask = UCP_OP_ATTR_FIELD_CALLBACK | UCP_OP_ATTR_FIELD_DATATYPE | UCP_OP_ATTR_FIELD_USER_DATA;
		param.datatype = ucp_dt_make_contig(1);
		param.user_data = &requestContext;
		param.cb.send = send_handler;

		request = ucp_tag_send_nbx(endpoint, message, message_length , 1, &param);

		request_finalize(worker, request, &requestContext, 0, iov);

	}
	else
	{ // client che riceve dal server
		peerInfo = client_handshake(argv[1], 12345, worker);
		endpoint = getEndpoint(worker, peerInfo, UCP_EP_PARAM_FIELD_REMOTE_ADDRESS);
		ucp_ep_print_info(endpoint, stdout);

		// list of buffers to store the data once recived. qua ne alloco uno solo
		// e init a zero la sua memoria
		ucp_dt_iov_t *iov = malloc(sizeof(ucp_dt_iov_t));
		memset(iov, 0, sizeof(*iov));

		ucp_request_param_t param;
		req_t *request;
		size_t message_length=strlen(__TEST_STRING__);


		param.op_attr_mask = UCP_OP_ATTR_FIELD_CALLBACK | UCP_OP_ATTR_FIELD_DATATYPE | UCP_OP_ATTR_FIELD_USER_DATA;
		param.datatype = ucp_dt_make_contig(1);
		param.user_data = &requestContext;

		param.cb.send = recv_handler;

		request = ucp_tag_recv_nbx(worker, message, message_length, 1, 0, &param);

		request_finalize(worker, request, &requestContext, 0, iov);

		printf("Recived: %s\n", message);
	}


	return 0;
}