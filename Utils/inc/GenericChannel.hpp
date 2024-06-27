/**
 * @file EventChannel.hpp
 * @author Matthew Da Silva (matthew.dasilva@wosler.ca)
 * @brief 
 * @version 0.1
 * @date 2023-02-24
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#pragma once

#include <vector>
#include <unordered_map>
#include <functional>
#include <algorithm>
#include <chrono>

#include "CommonUtilities.hpp"
#include "AsyncRunnable.hpp"
#include "IPCMessage.hpp"
#include "Event.hpp"
#include "IPCConfig.hpp"

// IPC Mechanism
#include "SocketInterface.hpp"

namespace wosler {
    namespace utilities {
        template <class RequestMessage, class ResponseMessage>
        class GenericChannel : protected AsyncRunnable{
        protected:
            const uint64_t channel_task_period_ms{0};

            const ModuleID m_modID;
            const ModuleConfig config;

            // IPC member variables
            std::unique_ptr<UDPSocketInterface> m_inputSocketInterface;
            std::unique_ptr<UDPSocketInterface> m_outputSocketInterface;
            sockaddr_in m_input_socket_config;
            sockaddr_in m_output_socket_config;
            sockaddr_in m_output_target_config;

            std::vector<char> raw_data_buffer;
            size_t m_buff_size;

            Error readMessage(std::unique_ptr<IPCMessage>& message, MessageType& msgType) {
                // std::cout << "Reading Message...\n";
                static sockaddr_in sender_config;
                static size_t sender_config_len = sizeof(sender_config);
                memset(&sender_config, 0, sizeof(sender_config));
                Error error = Error::NONE;

                ssize_t msg_size;

                // read from IPC method buffer
                if (m_inputSocketInterface.get())
                {
                    msg_size = m_inputSocketInterface->RecvPeek(
                        raw_data_buffer.data(),
                        m_buff_size,
                        reinterpret_cast<sockaddr *>(&sender_config),
                        reinterpret_cast<socklen_t *>(&sender_config_len));

                    // std::cout << "A message of " << (int)msg_size << " size was received...\n";
                    
                    error = m_inputSocketInterface->RecvFrom(
                        raw_data_buffer.data(),
                        msg_size,
                        reinterpret_cast<sockaddr *>(&sender_config),
                        reinterpret_cast<socklen_t *>(&sender_config_len));
                }

                if (error != Error::SUCCESS)
                {
                    std::cout << "Failed to receive data on socket: " + std::to_string(static_cast<int>(error)) + "\n";
                    return Error::FAILED;
                }
                else
                {
                    // std::cout << "Received Packet: ";
                    // for(int i = 0; (ssize_t) i < msg_size; i++) {
                    //     std::cout << std::hex << (unsigned int) raw_data_buffer[i] << " ";
                    // }
                    // std::cout << std::dec << " from " << inet_ntoa(sender_config.sin_addr) << ":" << ntohs(sender_config.sin_port) << "\n";
                    
                    msgType = (MessageType) raw_data_buffer[0];
                    if(msgType == MSG_TYPE_Request) {
                        message = std::make_unique<RequestMessage>();
                        message->Deserialize(raw_data_buffer.data(), m_buff_size);
                        // message->printMessage();
                    }
                    else if(msgType == MSG_TYPE_Response) {
                        message = std::make_unique<ResponseMessage>();
                        message->Deserialize(raw_data_buffer.data(), m_buff_size);
                        // message->printMessage();
                    }
                    else {
                        std::cout << "Message does not contain a valid message type!\n";
                        return Error::INVALID_STATE;
                    }  
                }
                return Error::SUCCESS;
            }

        public:
            GenericChannel(ModuleID mID, std::string name = "", uint64_t task_period_ms = 0): AsyncRunnable(name),
                                         channel_task_period_ms(task_period_ms), m_modID(mID), config(ModuleConfig_Map.at(mID))
            {
                // setup IPC mechanism
                RequestMessage msg;
                m_buff_size = msg.GetMessageSize()*10;

                ChannelSocketParam sockParam = config.socketParams.at(name);

                m_inputSocketInterface = std::make_unique<UDPSocketInterface>(
                    sockParam.ip,
                    static_cast<uint16_t>(sockParam.input_port),
                    m_buff_size);
                
                raw_data_buffer.resize(m_buff_size);

                Error error = Error::NONE;

                m_input_socket_config.sin_addr.s_addr = inet_addr(sockParam.ip.c_str());
                m_input_socket_config.sin_family = AF_INET;
                m_input_socket_config.sin_port = htons(sockParam.input_port);
                memset(&(m_input_socket_config.sin_zero), 0, 8);

                if (m_inputSocketInterface.get())
                {
                    // std::cout << name << " " << m_modID << " opening input socket at " << sockParam.ip << ":" << sockParam.input_port << "\n";
                    error = m_inputSocketInterface->Open();
                }

                if (error != wosler::utilities::Error::SUCCESS)
                {
                    std::cout << name << " " << m_modID << " failed to open input socket!\n";
                    throw 1;
                }

                if (m_inputSocketInterface.get())
                {
                    error = m_inputSocketInterface->Bind();
                }

                if (error != wosler::utilities::Error::SUCCESS)
                {
                    std::cout << name << " " << mID << " failed to bind input socket!\n";
                    throw 2;
                }

                m_outputSocketInterface = std::make_unique<UDPSocketInterface>(
                sockParam.ip,
                static_cast<uint16_t>(sockParam.output_port),
                m_buff_size);

                m_output_socket_config.sin_addr.s_addr = inet_addr(sockParam.ip.c_str());
                m_output_socket_config.sin_family = AF_INET;
                m_output_socket_config.sin_port = htons(sockParam.output_port);
                memset(&(m_output_socket_config.sin_zero), 0, 8);

                if (m_outputSocketInterface.get())
                {
                    // std::cout << name << " " << mID << " opening output socket at " << sockParam.ip << ":" << sockParam.output_port << "\n";
                    error = m_outputSocketInterface->Open();
                }

                if (error != wosler::utilities::Error::SUCCESS)
                {
                    std::cout << name << " " << mID << " failed to open output socket!\n";
                    throw 3;
                }
            }

            Error Start() {
                if(m_inputSocketInterface.get() && m_outputSocketInterface.get()) {
                    return RunTaskPeriodic(channel_task_period_ms);
                }
                else {
                    return Error::BAD_CONFIGURATION;
                }
            }

            Error Stop() {
                if(m_inputSocketInterface.get() && m_outputSocketInterface.get()) {
                    utilities::Error err = m_inputSocketInterface->Close();
                    if(err != utilities::Error::SUCCESS) {
                        return err;
                    }
                    return StopTask();
                }
                else {
                    return Error::BAD_CONFIGURATION;
                }
            }

            Error sendMessage(IPCMessage& message) {
                Error error = Error::NONE;
                
                // send message through IPC method buffer
                ModuleID target_mID = message.GetTargetModuleID();
                auto iter = ModuleConfig_Map.find(target_mID);
                if (iter != ModuleConfig_Map.end()) {
                    ModuleConfig target_config = iter->second;

                    ChannelSocketParam sockParam = target_config.socketParams.at(name);

                    m_output_target_config.sin_addr.s_addr = inet_addr(sockParam.ip.c_str());
                    m_output_target_config.sin_family = AF_INET;
                    m_output_target_config.sin_port = htons(sockParam.input_port);
                    memset(&(m_output_target_config.sin_zero), 0, 8);

                    size_t target_config_len = sizeof(m_output_socket_config);

                    std::vector<char> sendPacket;
                    sendPacket.resize(m_buff_size);

                    auto current_time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
                    auto current_time_msec = current_time.count();
                    message.SetTime(current_time_msec);

                    if (!message.Serialize(sendPacket.data(), m_buff_size))
                    {
                        std::cout << "Buffer size too small for packet!\n";
                        throw 4;
                        return Error::BAD_CONFIGURATION;
                    }
                    else{
                        if (m_outputSocketInterface.get())
                        {
                            // std::cout << "Sending packet: ";
                            // message.printMessage();
                            // std::cout << msg.GetMessageSize() << "\n";
                            // for(int i = 0; (size_t) i < msg.GetMessageSize(); i++) {
                            //     std::cout << std::hex << (unsigned int) sendPacket[i] << " ";
                            // }
                            // std::cout << std::dec << " to " << inet_ntoa(m_output_target_config.sin_addr) << ":" << ntohs(m_output_target_config.sin_port) << "\n";

                            error = m_outputSocketInterface->SendTo(
                                sendPacket.data(),
                                message.GetMessageSize(),
                                reinterpret_cast<sockaddr *>(&m_output_target_config),
                                reinterpret_cast<socklen_t *>(&target_config_len));
                        }

                        if (error != wosler::utilities::Error::SUCCESS)
                        {
                            std::cout << name << " " << m_modID << " failed to send data on socket: " + std::to_string(static_cast<int>(error)) + "\n";
                            throw 5;
                        }
                        return Error::SUCCESS;
                    }
                } else {
                    std::cout << "No valid target module was specified!\n";
                    throw 6;
                    return Error::BAD_CONFIGURATION;
                }
            }
        };
    }
}