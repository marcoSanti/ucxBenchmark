#define __DEBUG__

#include "ucxHelper.h"


#define __TEST_STRING__ "CIao questo è un sample di invio di messaggio!\0"


int main(int argc, char *argv[])
{

	req_t requestContext;
	requestContext.completed = 0;

	peerAddrInfo *peerInfo;
	ucp_ep_h endpoint;

	ucp_context_h context = bootstrapUcx(
		default_request_init,
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

		ucp_request_param_t* param = getTagSendReciveParametersSingle(
			UCP_OP_ATTR_FIELD_CALLBACK | UCP_OP_ATTR_FIELD_DATATYPE | UCP_OP_ATTR_FIELD_USER_DATA,
			&requestContext,
			default_send_handler
		);

		size_t message_length = strlen(__TEST_STRING__);
		

		req_t *request = ucp_tag_send_nbx(endpoint, message, message_length , 1, param);

		ucpWait(worker,request, &requestContext);

	}
	else
	{ // client che riceve dal server
		peerInfo = client_handshake(argv[1], 12345, worker);
		endpoint = getEndpoint(worker, peerInfo, UCP_EP_PARAM_FIELD_REMOTE_ADDRESS);
		ucp_ep_print_info(endpoint, stdout);

		//parameters for the request
		ucp_request_param_t* param = getTagSendReciveParametersSingle(
			UCP_OP_ATTR_FIELD_CALLBACK | UCP_OP_ATTR_FIELD_DATATYPE | UCP_OP_ATTR_FIELD_USER_DATA,
			&requestContext,
			default_recv_handler
		);


		size_t message_length=strlen(__TEST_STRING__);


		req_t *request = ucp_tag_recv_nbx(worker, message, message_length, 1, 0, param);

		
		ucpWait(worker,request, &requestContext);

		printf("Recived: %s\n", message);
	}


	return 0;
}