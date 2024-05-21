
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

/* Files required for transport initialization. */
#include <coresrv/nk/transport-kos.h>
#include <coresrv/sl/sl_api.h>

/* EDL description of the LightsGPIO entity. */
#include <traffic_light/LightsGPIO.edl.h>
#include <traffic_light/IStatus.idl.h>

#include <assert.h>
#include <pthread.h>

void *handle_transport(void *args);

/* Type of interface implementing object. */
typedef struct IModeImpl {
    struct traffic_light_IMode base;     /* Base interface of object */
    rtl_uint32_t step;                   /* Extra parameters */
} IModeImpl;

typedef struct IStatusImpl {
    struct traffic_light_IStatus base;     /* Base interface of object */
    rtl_uint32_t status;                   /* Extra parameters */
} IStatusImpl;

/* Mode method implementation. */
static nk_err_t FMode_impl(struct traffic_light_IMode *self,
                          const struct traffic_light_IMode_FMode_req *req,
                          const struct nk_arena *req_arena,
                          traffic_light_IMode_FMode_res *res,
                          struct nk_arena *res_arena)
{
    IModeImpl *impl = (IModeImpl *)self;
    /**
     * Increment value in control system request by
     * one step and include into result argument that will be
     * sent to the control system in the lights gpio response.
     */
    res->result = req->value + impl->step;
    return NK_EOK;
}

static nk_err_t GetStatus_impl(struct traffic_light_IStatus *self,
                          const struct traffic_light_IStatus_GetStatus_req *req,
                          const struct nk_arena *req_arena,
                          traffic_light_IStatus_GetStatus_res *res,
                          struct nk_arena *res_arena)
{
    IStatusImpl *impl = (IStatusImpl *)self;
    
    // res-> = impl->status;
    return NK_EOK;
}

static struct traffic_light_IStatus *CreateIStatusImpl(rtl_uint32_t status)
{
    /* Table of implementations of IMode interface methods. */
    static const struct traffic_light_IStatus_ops ops = {
        .GetStatus = GetStatus_impl
    };

    /* Interface implementing object. */
    static struct IStatusImpl impl = {
        .base = {&ops}
    };

    impl.status = status;

    return &impl.base;
}

/**
 * IMode object constructor.
 * step is the number by which the input value is increased.
 */
static struct traffic_light_IMode *CreateIModeImpl(rtl_uint32_t step)
{
    /* Table of implementations of IMode interface methods. */
    static const struct traffic_light_IMode_ops ops = {
        .FMode = FMode_impl
    };

    /* Interface implementing object. */
    static struct IModeImpl impl = {
        .base = {&ops}
    };

    impl.step = step;

    return &impl.base;
}

typedef struct {
    NkKosTransport *transport;
    traffic_light_LightsGPIO_entity *entity;
    traffic_light_LightsGPIO_entity_req *gpioRequest;
    struct nk_arena *req_arena;
    struct nk_arena *res_arena;
    traffic_light_LightsGPIO_entity_res *gpioResult;
} ThreadArgs;

void *handle_transport(void *args) {
    fprintf(stderr, "Starting thread\n");
    ThreadArgs *threadArgs = (ThreadArgs *)args;

    while (true) {
        /* Flush request/response buffers. */
        nk_req_reset(threadArgs->gpioRequest);
        nk_arena_reset(threadArgs->req_arena);
        nk_arena_reset(threadArgs->res_arena);

        /* Wait for request to lights gpio entity. */
        if (nk_transport_recv(&threadArgs->transport->base,
                              &threadArgs->gpioRequest->base_,
                              threadArgs->req_arena) != NK_EOK) {
            fprintf(stderr, "nk_transport_recv error\n");
        } else {
            /**
             * Handle received request by calling implementation Mode_impl
             * of the requested Mode interface method.
             */
            traffic_light_LightsGPIO_entity_dispatch(threadArgs->entity, &threadArgs->gpioRequest->base_, threadArgs->req_arena,
                                        &threadArgs->gpioResult->base_, threadArgs->res_arena);
        }

        /* Send response. */
        if (nk_transport_reply(&threadArgs->transport->base,
                               &threadArgs->gpioResult->base_,
                               threadArgs->res_arena) != NK_EOK) {
            fprintf(stderr, "[mode] nk_transport_reply error\n");
        }
    }

    return NULL;
}

int main(void)
{
    ServiceId iid;
    Handle handle = ServiceLocatorRegister("lights_gpio_connection", NULL, 0, &iid);
    assert(handle != INVALID_HANDLE);

    NkKosTransport transport;
    NkKosTransport_Init(&transport, handle, NK_NULL, 0);

    ServiceId iidDiag;
    Handle handleDiag = ServiceLocatorRegister("diagnostics_connection", NULL, 0, &iidDiag);
    assert(handleDiag != INVALID_HANDLE);

    NkKosTransport transportDiag;
    NkKosTransport_Init(&transportDiag, handleDiag, NK_NULL, 0);

    traffic_light_LightsGPIO_entity_req gpioRequest;
    char gpioReqBuffer[traffic_light_LightsGPIO_entity_req_arena_size];
    struct nk_arena req_arena = NK_ARENA_INITIALIZER(gpioReqBuffer,
                                        gpioReqBuffer + sizeof(gpioReqBuffer));

    traffic_light_LightsGPIO_entity_res gpioResult;
    char res_buffer[traffic_light_LightsGPIO_entity_res_arena_size];
    struct nk_arena res_arena = NK_ARENA_INITIALIZER(res_buffer,
                                        res_buffer + sizeof(res_buffer));

    traffic_light_CMode_component component;
    traffic_light_CMode_component_init(&component, CreateIModeImpl(0x1000000));

    traffic_light_CStatus_component componentSts;
    traffic_light_CStatus_component_init(&componentSts, CreateIStatusImpl(0x1000000));

    /* Initialize lights gpio entity dispatcher. */
    traffic_light_LightsGPIO_entity entity;
    traffic_light_LightsGPIO_entity_init(&entity, &component, &componentSts);

    fprintf(stderr, "Hello I'm LightsGPIO\n");

    ThreadArgs threadArgs1 = {&transport, &entity, &gpioRequest, &req_arena, &res_arena, &gpioResult};
    ThreadArgs threadArgs2 = {&transportDiag, &entity, &gpioRequest, &req_arena, &res_arena, &gpioResult};

    pthread_t thread1, thread2;

    pthread_create(&thread1, NULL, handle_transport, &threadArgs1);
    pthread_create(&thread2, NULL, handle_transport, &threadArgs2);

    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);

    return EXIT_SUCCESS;
}
