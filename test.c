#include "ucxHelper.h"

void request_init(void *request)
{
    struct ucx_context *ctx = (struct ucx_context *)request;
    ctx->completed = 0;
}

void main()
{

    ucp_context_h context = bootstrapUcx(
        request_init,
        UCP_PARAM_FIELD_FEATURES | UCP_PARAM_FIELD_REQUEST_SIZE | UCP_PARAM_FIELD_REQUEST_INIT,
        UCP_FEATURE_TAG);
    ucp_context_print_info(context, stdout);


    ucp_worker_h worker = getUcxWorker(
        context,
        UCP_WORKER_PARAM_FIELD_THREAD_MODE, 
        UCS_THREAD_MODE_SINGLE );

    ucp_worker_print_info(worker, stdout);

    

}