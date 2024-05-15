#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <stdint.h>
#include <iostream>

#include <coresrv/nk/transport-kos.h>
#include <coresrv/sl/sl_api.h>
#include <stdlib.h>
#include <time.h>

#include <traffic_light/Diagnostics.edl.h>
#include <traffic_light/IStatus.idl.h>
#include <unistd.h>

namespace{
constexpr const char* entityName{"Diagnostics"};
constexpr const char* connectionName{"diagnostics_connection"};
constexpr const char* getStatusMethodId{"traffic_light.LightsGPIO.GetStatus"};
constexpr const int diagPollPeriodS{1};
}

static uint32_t currentStatus{0x00};

/* Diagnostics entry point. */
int main(int argc, const char *argv[])
{
    std::cerr << "Hello, I'm " << entityName << "\n";

    Handle handleDiag = ServiceLocatorConnect(connectionName);
    if (handleDiag == INVALID_HANDLE)
    {
        std::cerr << "[" << entityName << "] " << "Error: can`t establish static IPC connection\n";
        return EXIT_FAILURE;
    }

    NkKosTransport transport;
    NkKosTransport_Init(&transport, handleDiag, NK_NULL, 0);

    nk_iid_t riid = ServiceLocatorGetRiid(handleDiag, getStatusMethodId);
    if (riid == INVALID_RIID)
    {
        std::cerr << "[" << entityName << "] " << "Error: can`t get runtime implementation ID (RIID) of "<< getStatusMethodId << "\n";
        return EXIT_FAILURE;
    }
    
    struct traffic_light_IStatus_proxy statusProxy;
    traffic_light_IStatus_proxy_init(&statusProxy, &transport.base, riid);

    traffic_light_IStatus_GetStatus_req request;
    traffic_light_IStatus_GetStatus_res result;

    while (true)
    {
        sleep(diagPollPeriodS);
        std::cerr << "[" << entityName << "] " << "Performing diag IPC request\n";
        if (traffic_light_IStatus_GetStatus(&statusProxy.base, &request, nullptr, &result, nullptr) != rcOk)
        {
            // currentStatus = result.result;
        }
        else
        {
            std::cerr << "[" << entityName << "] " << "Failed to call" << getStatusMethodId << "\n";
        }
    }

    return EXIT_SUCCESS;
}
