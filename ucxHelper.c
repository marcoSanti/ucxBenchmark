#include "ucxHelper.h"


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
    ucs_status_t status;


    memset(&ucp_params, 0, sizeof(ucp_params));

    ucp_params.field_mask = ucpParamsFields;
    ucp_params.features = ucpFeature;
    ucp_params.request_size = sizeof(struct ucx_context); // The size of a reserved space in a non-blocking requests. Typically applications use this space for caching own structures in order to avoid costly memory allocations, pointer dereferences, and cache misses.
    ucp_params.request_init = request_init;

    status = ucp_config_read(NULL, NULL, &ucp_config);

    // inizializzo il contesto ucp, ottenendo in output il contestop
    status = ucp_init(&ucp_params, ucp_config, &ucp_context);

#ifdef __DEBUG__
    ucp_config_print(ucp_config, stdout, NULL, UCS_CONFIG_PRINT_CONFIG);
#endif
    ucp_config_release(ucp_config); // poiche ho il contesto, non mi serve avere
                                    // avere ancora in memoria la config e la posso liberare

    return ucp_context;
}



ucp_worker_h getUcxWorker(ucp_context_h context, uint64_t field_mask, ucs_thread_mode_t thread_mode){
    ucp_worker_params_t worker_params;
    ucp_worker_h ucp_worker;
    ucs_status_t status;

    memset(&worker_params, 0, sizeof(worker_params));

    worker_params.field_mask = UCP_WORKER_PARAM_FIELD_THREAD_MODE; // BOH NO SO CHE SERVE> dice che serve per retrocompatibilita ma bog
    worker_params.thread_mode = UCS_THREAD_MODE_SINGLE;            // see

    status = ucp_worker_create(context, &worker_params, &ucp_worker);

    return ucp_worker;
}

