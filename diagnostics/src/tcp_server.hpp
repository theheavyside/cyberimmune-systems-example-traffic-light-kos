#pragma once

#include <cstdint>
#include <iostream>
#include <boost/asio.hpp>

namespace net = boost::asio;
namespace sys = boost::system;

//for compatibility with fprintf in *.c files
#define ENDL "\n";

class TcpServer
{
public:
    explicit TcpServer(int port) 
        : m_port{port}
        , m_acceptor{m_ioContext, 
                     net::ip::tcp::endpoint(net::ip::tcp::v4(), m_port)}
    {
        std::cerr << "Server started. Listening on port: " << m_port << ENDL;
    }

    void Loop(uint8_t status)
    {
        net::ip::tcp::socket socket{m_ioContext};
        m_acceptor.accept(socket);
        
        std::cerr << "New connection accepted" << ENDL;
        HandleRequest(socket, status);
    }

private:
    void HandleRequest(net::ip::tcp::socket& socket, uint8_t status) {
        try {
            uint8_t buffer[1024];
            sys::error_code error;
            
            size_t bytesTransferred = socket.read_some(net::buffer(buffer), error);
            
            if (!error) {
                if (bytesTransferred == 1 && buffer[0] == 0x99) {
                    uint8_t response{status};
                    net::write(socket, net::buffer(&response, 1), error);
                } else {
                    std::cerr << "Invalid request!" << ENDL;
                }
            } else {
                std::cerr << "Error on receive: " << error.message() << ENDL;
            }
        } catch (std::exception& e) {
            std::cerr << "Exception: " << e.what() << ENDL;
        }
    }

private:
    net::io_context m_ioContext;
    net::ip::tcp::acceptor m_acceptor;
    int m_port;
};
