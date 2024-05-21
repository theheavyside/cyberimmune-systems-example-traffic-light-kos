#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <stdint.h>
#include <iostream>

#include "tcp_server.hpp"

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
constexpr const char* getStatusMethodId{"diagStatus.status"};
constexpr const int diagPollPeriodSeconds{1};
constexpr const int tcpServerPort{7777};
}

static uint32_t currentStatus{0x00};

/* Diagnostics entry point. */
int main(int argc, const char *argv[])
{
    std::cerr << "Hello, I'm " << entityName << ENDL;

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
        std::cerr << "[" << entityName << "] " << "Error: can`t get runtime implementation ID (RIID) of "<< getStatusMethodId << ENDL;
        return EXIT_FAILURE;
    }
    
    struct traffic_light_IStatus_proxy statusProxy;
    traffic_light_IStatus_proxy_init(&statusProxy, &transport.base, riid);

    traffic_light_IStatus_GetStatus_req request;
    traffic_light_IStatus_GetStatus_res result;

    try {
        TcpServer server{tcpServerPort};

        while (true)
        {
            sleep(diagPollPeriodSeconds);
            std::cerr << "[" << entityName << "] " << "Performing diag IPC request\n";

            if (traffic_light_IStatus_GetStatus(&statusProxy.base, &request, NULL, &result, NULL) == rcOk)
            {
                currentStatus = result.status;
            }
            else
            {
                std::cerr << "[" << entityName << "] " << "Failed to call" << getStatusMethodId << ENDL;
            }
            
            server.Loop(currentStatus);
        }
    }
    catch(const std::exception& e)
    {
        std::cerr << "Caught error: " << e.what() << ENDL;
        std::cerr << "Exiting " << entityName << " entity" << ENDL;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
