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
#include "GenericChannel.hpp"
#include "IPCMessage.hpp"
#include "Event.hpp"
#include "IPCConfig.hpp"

// IPC Mechanism
#include "SocketInterface.hpp"

namespace wosler {
    namespace utilities {
        class StateChannel : public GenericChannel<StateRequestMessage,StateResponseMessage>{
        private:
            bool m_isClient;

            // state callback functions
            std::function<void(StateRequestMessage&,StateResponseMessage&)> m_stateChangeCallback;
            std::function<void(StateRequestMessage&,StateResponseMessage&)> m_stateResponseCallback;

            void OnProcess() override {
                std::unique_ptr<IPCMessage> message; 
                MessageType msgType;
                Error error = readMessage(message, msgType);

                if(error != Error::SUCCESS) {
                    std::cout << "Failed to read message!\n";
                }
                else {
                    if(m_isClient){
                        if (msgType == MSG_TYPE_Response) {
                            std::cout << "Received a response message as a Client channel!\n\n";
                            throw 1;
                        }
                        // std::cout << "\nReading Request...";
                        StateRequestMessage* requestMessage = static_cast<StateRequestMessage*>(message.get());
                        // requestMessage->printMessage();
                        StateResponseMessage response;
                        response.SetModuleID(m_modID);
                        response.SetTargetModuleID(requestMessage->GetModuleID());

                        // perform necessary operations and call event
                        m_stateChangeCallback(*requestMessage, response);
                        try {
                            sendMessage(response);
                        }
                        catch(std::exception const& e) {
                            std::cout << e.what() << "\n";
                        }
                    }
                    else {
                        if (msgType == MSG_TYPE_Request) {
                            std::cout << "Received a request message as a Server channel!\n\n";
                            throw 2;
                        }
                        // std::cout << "\nReading Response...";
                        StateResponseMessage* responseMessage = static_cast<StateResponseMessage*>(message.get());
                        // responseMessage->printMessage();
                        StateRequestMessage request;
                        m_stateResponseCallback(request, *responseMessage);
                    }
                }
            }

        public:
            StateChannel(ModuleID mID, bool isClient = false) : GenericChannel<StateRequestMessage,StateResponseMessage>(mID, "State Channel",10),
                                                                m_isClient(isClient) { }

            void addStateChangeCallback(std::function<void(StateRequestMessage&, StateResponseMessage&)> function) {
                if(m_isClient) {
                    // link the function to the state change callback
                    m_stateChangeCallback = function;
                }
                else {
                    std::cout << "Can't register a state change callback for a Server channel!\n\n";
                    throw 3;
                }
            }

            void addStateResponseCallback(std::function<void(StateRequestMessage&, StateResponseMessage&)> function) {
                if(!m_isClient) {
                    // link the function to the state change callback
                    m_stateResponseCallback = function;
                }
                else {
                    std::cout << "Can't register a state response callback for a Client channel!\n\n";
                    throw "Can't register a state response callback for a Client channel!\n";
                }
            }
        };
    }
}