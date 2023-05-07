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
#include <fcntl.h>




/**
 * Functions to make more easy to develop ucx applications
 */

typedef struct _peerAddrInfo
{
    uint64_t addr_len;
    ucp_address_t *peer_addr;
} peerAddrInfo;

/**
 * @brief Thif structure is used to define wheter a request has been served or not
 * 
 */
typedef struct _req {
    int completed;
} req_t;

/**
 * @brief Functions to check for errors in UXC api calls
 *
 */
#define CHKERR_ACTION(_cond, _msg, _action)          \
    do                                               \
    {                                                \
        if (_cond)                                   \
        {                                            \
            fprintf(stderr, "Failed to %s\n", _msg); \
            _action;                                 \
        }                                            \
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

peerAddrInfo *server_handshake(uint16_t server_port, ucp_worker_h worker);

peerAddrInfo *client_handshake(const char *server, uint16_t server_port, ucp_worker_h worker);

ucp_ep_h getEndpoint(ucp_worker_h worker, peerAddrInfo *peer, uint64_t ep_field_mask);

ucs_status_t ucpWait(ucp_worker_h ucp_worker, void *request, req_t* ctx);

void waitForMessageWithTag(ucp_worker_h worker, ucp_tag_t messageTag, ucp_tag_t messageTagMask, ucp_tag_recv_info_t* incoming_msg);

ucp_request_param_t *getTagSendReciveParametersSingle(uint32_t parameterMask, req_t* requestContext, void* callbackFunctionHandle);

void default_recv_handler(void *request, ucs_status_t status, const ucp_tag_recv_info_t *info, void *user_data);

void default_send_handler(void *request, ucs_status_t status, void *ctx);

void default_request_init(void *request);


//test for infiniband problem fix
void device_handshake(const char *peer_addr, uint16_t server_port, ucp_worker_h worker);
peerAddrInfo *host_handshake(uint16_t server_port);

#endif
