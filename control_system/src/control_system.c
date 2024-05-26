
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

/* Files required for transport initialization. */
#include <coresrv/nk/transport-kos.h>
#include <coresrv/sl/sl_api.h>

/* Description of the lights gpio interface used by the `ControlSystem` entity. */
#include <strict/posix/unistd.h>
#include <traffic_light/IMode.idl.h>

#include <assert.h>

#define MODES_NUM 8
#define SWITCH_MODE_PERIOD_S 1
#define LOG_ID "ControlSys"

/* Control system entity entry point. */
int main(int argc, const char *argv[])
{
    (void)argv[0];
    (void)argc;

    NkKosTransport transport;
    struct traffic_light_IMode_proxy proxy;

    static const nk_uint32_t tl_modes[MODES_NUM] = {
        traffic_light_IMode_Direction1Red + traffic_light_IMode_Direction2Red,
        traffic_light_IMode_Direction1Red + traffic_light_IMode_Direction1Yellow + traffic_light_IMode_Direction2Red,
        traffic_light_IMode_Direction1Green + traffic_light_IMode_Direction2Red,
        traffic_light_IMode_Direction1Yellow + traffic_light_IMode_Direction2Red,
        traffic_light_IMode_Direction1Red + traffic_light_IMode_Direction2Yellow + traffic_light_IMode_Direction2Red,
        traffic_light_IMode_Direction1Red + traffic_light_IMode_Direction2Green,
        traffic_light_IMode_Direction1Red + traffic_light_IMode_Direction2Yellow,
        traffic_light_IMode_Direction1Yellow + traffic_light_IMode_Direction1Blink + traffic_light_IMode_Direction2Yellow + traffic_light_IMode_Direction2Blink
    };

    fprintf(stderr, "Hello I'm ControlSystem\n");

    /**
     * Get the LightsGPIO IPC handle of the connection named
     * "lights_gpio_connection".
     */
    Handle handle = ServiceLocatorConnect("lights_gpio_connection");
    assert(handle != INVALID_HANDLE);

    /* Initialize IPC transport for interaction with the lights gpio entity. */
    NkKosTransport_Init(&transport, handle, NK_NULL, 0);

    /**
     * Get Runtime Interface ID (RIID) for interface traffic_light.Mode.mode.
     * Here mode is the name of the traffic_light.Mode component instance,
     * traffic_light.Mode.mode is the name of the Mode interface implementation.
     */
    nk_iid_t riid = ServiceLocatorGetRiid(handle, "lightsGpio.mode");
    assert(riid != INVALID_RIID);

    /**
     * Initialize proxy object by specifying transport (&transport)
     * and lights gpio interface identifier (riid). Each method of the
     * proxy object will be implemented by sending a request to the lights gpio.
     */
    traffic_light_IMode_proxy_init(&proxy, &transport.base, riid);

    /* Request and response structures */
    traffic_light_IMode_FMode_req req;
    traffic_light_IMode_FMode_res res;

    /* Test loop. */
    req.value = 0;
    int i = 0;
    while(1)
    {
        req.value = tl_modes[i];
        /**
         * Call Mode interface method.
         * Lights GPIO will be sent a request for calling Mode interface method
         * mode_comp.mode_impl with the value argument. Calling thread is locked
         * until a response is received from the lights gpio.
         */
        if (traffic_light_IMode_FMode(&proxy.base, &req, NULL, &res, NULL) == rcOk)
        {
            /**
             * Print result value from response
             * (result is the output argument of the Mode method).
             */
            fprintf(stderr, "[%s] request = %0x; result = %0x\n", LOG_ID, (int)req.value, (int) res.result);
            /**
             * Include received result value into value argument
             * to resend to lights gpio in next iteration.
             */
            req.value = res.result;

            sleep(SWITCH_MODE_PERIOD_S);

            i = (i + 1) % MODES_NUM;
        }
        else
        {
            fprintf(stderr, "[%s] Failed to call traffic_light.Mode.Mode()\n", LOG_ID);
            return EXIT_FAILURE;
        }

    }

    return EXIT_SUCCESS;
}
