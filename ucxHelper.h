#ifndef __UCX_INCLUDES_
    #define __UCX_INCLUDES_
    #include <ucs/memory/memory_type.h>
    #include <ucp/api/ucp.h>
    #include <unistd.h>
    #include <sys/socket.h>
    #include <sys/types.h>
    #include <sys/epoll.h>
    #include <netinet/in.h>
    #include <assert.h>
    #include <netdb.h>
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <ctype.h>   /* isprint */
    #include <pthread.h> /* pthread_self */
    #include <errno.h>   /* errno */
    #include <time.h>

    /**
     * Functions to make more easy to develop ucx applications
     */

 
    // TODO: vedere dove voene usato
    struct ucx_context
    {
        int completed;
    };
    /**
     * @brief Functions to check for errors in UXC api calls
     * 
     */
    #define CHKERR_ACTION(_cond, _msg, _action) \
        do { \
            if (_cond) { \
                fprintf(stderr, "Failed to %s\n", _msg); \
                _action; \
            } \
        } while (0)


    /**
     * @brief Thif function creates a ucp context and returns it after initializing ucx. 
     * 
     * @param request_init the function used to initialize a request (GUARDARE CHE NON SO A CHE SERVA)
     * @param ucpParamsFields Bitmask of wich features the ucp program will use
     * @param ucpFeature The type of comunication we want to use (ex: tag based or rma)
     * @return ucp_context_h the context for the ucx
     */
    ucp_context_h bootstrapUcx(ucp_request_init_callback_t request_init, enum ucp_params_field ucpParamsFields, enum ucp_feature ucpFeature);



    ucp_worker_h getUcxWorker(ucp_context_h context, uint64_t field_mask, ucs_thread_mode_t thread_mode);



#endif
