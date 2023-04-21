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
}