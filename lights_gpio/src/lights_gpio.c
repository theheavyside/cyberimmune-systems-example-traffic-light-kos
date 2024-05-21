
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

/* Lights GPIO entry point. */
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

    /* Dispatch loop implementation. */
    do
    {
        /* Flush request/response buffers. */
        nk_req_reset(&gpioRequest);
        nk_arena_reset(&req_arena);
        nk_arena_reset(&res_arena);

        /* Wait for request to lights gpio entity. */
        if (nk_transport_recv(&transport.base,
                              &gpioRequest.base_,
                              &req_arena) != NK_EOK) {
            fprintf(stderr, "[mode] nk_transport_recv error\n");
        } else {
            /**
             * Handle received request by calling implementation Mode_impl
             * of the requested Mode interface method.
             */
            traffic_light_LightsGPIO_entity_dispatch(&entity, &gpioRequest.base_, &req_arena,
                                        &gpioResult.base_, &res_arena);
        }

        /* Wait for request to lights gpio entity. */
        if (nk_transport_recv(&transportDiag.base,
                              &gpioRequest.base_,
                              &req_arena) != NK_EOK) {
            fprintf(stderr, "[diag] nk_transport_recv error\n");
        } else {
            /**
             * Handle received request by calling implementation Mode_impl
             * of the requested Mode interface method.
             */
            traffic_light_LightsGPIO_entity_dispatch(&entity, &gpioRequest.base_, &req_arena,
                                        &gpioResult.base_, &res_arena);
        }

        /* Send response. */
        if (nk_transport_reply(&transport.base,
                               &gpioResult.base_,
                               &res_arena) != NK_EOK) {
            fprintf(stderr, "[mode] nk_transport_reply error\n");
        }

        /* Send diag response. */
        if (nk_transport_reply(&transportDiag.base,
                               &gpioResult.base_,
                               &res_arena) != NK_EOK) {
            fprintf(stderr, "[diag] nk_transport_reply error\n");
        }
    }
    while (true);

    return EXIT_SUCCESS;
}
