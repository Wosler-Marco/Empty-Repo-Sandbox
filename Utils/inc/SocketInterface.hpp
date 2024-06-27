/**
 * @file SocketInterface.hpp
 * @author Jai Sood (ankur.jai.sood@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2022-04-15
 * 
 * @copyright Copyright (c) 2022 Wosler Inc.
 * 
 */

#pragma once

#include <iostream>
#include <string>
#include <cstring>
#include <vector>

// sys/socket.h -> Winsock2.h
// netinet/in.h -> Ws2tcpip.h (inet_addr -> inet_pton)
// arpa/inet.h -> Winsock2.h (inet_addr)
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <io.h>
#define SockBufferType char*
typedef SSIZE_T ssize_t;
#endif

#if defined(__linux__) || defined(__arm__)
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#define SockBufferType void*
#endif

#include <sys/types.h>

#include <errno.h>

#include <errno.h>

#include "CommonUtilities.hpp"

namespace wosler {
namespace utilities {

class UDPSocketInterface {
    private:
    int socket_fd{-1};
    const std::string ip_addr{};
    const uint16_t port_number{0};
    const uint8_t length{0};
    sockaddr_in socket_config;

    #ifdef _WIN32
    WSADATA wsaData = {0};
    int iResult = 0;
    #endif

    public:
    UDPSocketInterface(
        const std::string& ip_address,
        const uint16_t port_num,
        const uint8_t num_bytes) :
            ip_addr(ip_address),
            port_number(port_num),
            length(num_bytes) 
    {
        memset(static_cast<void*>(&socket_config), 0, sizeof(socket_config));

        socket_config.sin_addr.s_addr = inet_addr(ip_addr.data());
        socket_config.sin_family = AF_INET;
        socket_config.sin_port = htons(port_number);
    };

    ~UDPSocketInterface() {
        Close();
    };

    Error Open() {
        if(socket_fd == -1) {
            #ifdef _WIN32
            iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
            if (iResult != 0) {
                std::cout << "WSAStartup failed: " << iResult << "\n";
                return Error::FAILED;
            }
            #endif
            socket_fd = socket(socket_config.sin_family, SOCK_DGRAM, 0);
            
            int reuse = 1;
            int result = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse));
            #ifdef _WIN32
            if (result == SOCKET_ERROR)
            #else
            if (result == -1)
            #endif
            {
                return Error::FAILED;
            }

            return socket_fd >= 0 ? Error::SUCCESS : Error::FAILED;
        }
        return Error::ALREADY_OPEN;
    };

    Error Bind() {
        if(socket_fd >= 0) {
            int result = bind(
                socket_fd,
                reinterpret_cast<sockaddr*>(&socket_config),
                sizeof(socket_config)
            );
            
            if(result == -1)
            {
                std::cout << "Error while binding socket: " << strerror(errno) << "\n";
            }

            return result == 0 ? Error::SUCCESS : Error::FAILED;
        }
        return Error::INVALID_STATE;
    };

    Error Close() {
        if(socket_fd >= 0) {
            #ifdef _WIN32
            int result = closesocket(socket_fd);
            #elif defined(__linux__) || defined(__arm__)
            int result = close(socket_fd);
            #endif
            if(result != 0) {
                std::cout << "Error while closing socket: " << strerror(errno) << "\n";
            }
            return result == 0 ? Error::SUCCESS : Error::FAILED;
        }
        return Error::INVALID_STATE;
    };

    Error RecvFrom(
        SockBufferType recvBuffer,
        const size_t num_bytes,
        sockaddr* sender_info = nullptr,
        socklen_t* sender_info_len = nullptr,
        int timeout_msec = -1)
    {
        if(!recvBuffer) {return Error::BAD_PARAMETER;}
        
        if(socket_fd >= 0) {
            fd_set read_fds;
            FD_ZERO(&read_fds);
            FD_SET(socket_fd, &read_fds);

            timeval timeout;
            timeout.tv_sec = timeout_msec/1000;
            timeout.tv_usec = (timeout_msec%1000)*1000;

            int select_result = select(socket_fd + 1, &read_fds, NULL, NULL, timeout_msec > 0 ? &timeout : nullptr);
            if (select_result == -1) {
                std::cout << "Error while waiting for incoming data: " << strerror(errno) << "\n";
                return Error::FAILED;
            } else if (select_result == 0) {
                std::cout << "Timeout waiting for incoming data\n";
                return Error::TIMEOUT;
            } else {
                ssize_t num_bytes_received = recvfrom(
                    socket_fd,
                    recvBuffer,
                    num_bytes,
                    0,
                    sender_info,
                    sender_info_len
                );
                
                if((size_t)num_bytes_received != num_bytes) {return Error::FAILED;}
                return Error::SUCCESS;
            }
        }
        return Error::INVALID_STATE;
    };

    ssize_t RecvPeek(
        SockBufferType recvBuffer,
        const size_t num_bytes,
        sockaddr* sender_info = nullptr,
        socklen_t* sender_info_len = nullptr,
        int timeout_msec = -1)
    {
        if(!recvBuffer) {return -(int)Error::BAD_PARAMETER;}
        
        if(socket_fd >= 0) {
            fd_set read_fds;
            FD_ZERO(&read_fds);
            FD_SET(socket_fd, &read_fds);

            timeval timeout;
            timeout.tv_sec = timeout_msec/1000;
            timeout.tv_usec = (timeout_msec%1000)*1000;

            int select_result = select(socket_fd + 1, &read_fds, NULL, NULL, timeout_msec > 0 ? &timeout : nullptr);
            if (select_result == -1) {
                std::cout << "Error while waiting for incoming data: " << strerror(errno) << "\n";
                return -(int)Error::FAILED;
            } else if (select_result == 0) {
                std::cout << "Timeout waiting for incoming data\n";
                return -(int)Error::TIMEOUT;
            } else {
            ssize_t num_bytes_received = recvfrom(
                socket_fd,
                recvBuffer,
                num_bytes,
                MSG_PEEK,
                sender_info,
                sender_info_len
            );
            return num_bytes_received;
            }
        }
        return -(int)Error::INVALID_STATE;
    };

    Error SendTo(
        const SockBufferType sendBuffer,
        const size_t num_bytes,
        const sockaddr* recv_info = nullptr,
        socklen_t* recv_info_len = nullptr
    ) 
    {
        if(!sendBuffer) {return Error::BAD_PARAMETER;}
        if(socket_fd >= 0) {
            ssize_t num_bytes_sent = sendto(
                socket_fd,
                sendBuffer,
                num_bytes,
                0,
                recv_info,
                recv_info == nullptr ? 0 : sizeof(*recv_info)
            );
            if(num_bytes_sent == -1)
            {
                printf("Error number: %d ", errno);
                std::cout << strerror(errno) << "\n";
            }
            
            if((size_t)num_bytes_sent != num_bytes) {return Error::FAILED;}
            return Error::SUCCESS;
        }
        return Error::INVALID_STATE;
    };

    const std::string& getIPAddr() const {
        return ip_addr;
    };

    const uint16_t& getPort() const {
        return port_number;
    };
};

} // end namespace wosler
} // end namespace utilities
