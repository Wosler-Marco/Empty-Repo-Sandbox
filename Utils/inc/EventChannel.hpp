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
        enum ChannelFunctions {
            Subscribe = 1,
            Update,
            Query
        };

        class EventChannel : public GenericChannel<EventRequestMessage,EventResponseMessage>{
        private:
            // event member variables
            std::vector<EventID> m_events;
            std::unordered_map<EventID, std::function<void(EventRequestMessage&,EventResponseMessage&)>> m_eventCallback;
            std::unordered_map<EventID, std::vector<ModuleID>> m_eventSubscribers;

            void OnProcess() override {
                std::unique_ptr<IPCMessage> message; 
                MessageType msgType;
                Error error = readMessage(message, msgType);
                
                if(error != Error::SUCCESS) {
                    std::cout << "Failed to read message!\n";
                }
                else {
                    if(msgType == MSG_TYPE_Request){
                        // std::cout << "\nReading Request...";
                        EventRequestMessage* requestMessage = static_cast<EventRequestMessage*>(message.get());
                        // requestMessage->printMessage();

                        switch(requestMessage->GetFunctionID()) {
                            case (FunctionID) Subscribe:
                                subscribeToEvent(requestMessage->GetEventID(), requestMessage->GetModuleID());
                                break;
                            case (FunctionID) Update:
                                updateEvent(*requestMessage);
                                break;
                            case (FunctionID) Query:
                                queryEvents(requestMessage->GetModuleID());
                                break;
                            default:
                                std::cout << "Invalid Function ID!\n\n";
                        }
                    }
                    else if(msgType == MSG_TYPE_Response) {
                        // std::cout << "\nReading Response...";
                        EventResponseMessage* responseMessage = static_cast<EventResponseMessage*>(message.get());
                        // responseMessage->printMessage();

                        updateEvent(*responseMessage);
                    }
                    else { std::cout << "Invalid message type...\n"; }
                }
            }

            void subscribeToEvent(EventID event, ModuleID process) {
                // add process to the list of the specified event's subscribers
                // check if process is already in list of subscribers
                // std::cout << "Subscribing module " << process << " to event " << event << "\n";
                if(std::find(m_eventSubscribers[event].begin(), m_eventSubscribers[event].end(), process) == m_eventSubscribers[event].end()) {
                    m_eventSubscribers[event].push_back(process);
                }
            }

            void updateEvent(IPCMessage& message) {
                if(message.GetMessageType() == MessageType::MSG_TYPE_Response) {
                    // std::cout << "+++Running response callback...\n";
                    EventResponseMessage* msg = static_cast<EventResponseMessage*>(&message);
                    EventID event = msg->GetEventID();
                    EventRequestMessage request;

                    // perform necessary operations and call event
                    if(m_eventCallback.find(event) != m_eventCallback.end()) {
                        m_eventCallback[event](request, *msg);
                    }
                    else {
                        std::cout << "Module " << m_modID << " callback not defined for this event ID - " << event << "!\n";
                    }
                }
                else {
                    EventRequestMessage* msg = static_cast<EventRequestMessage*>(&message);
                    // break down message into individual parts
                    EventID event = msg->GetEventID();
                    // based on eventID, perform the functions listed in m_eventCallbacks
                    EventResponseMessage response;
                    std::chrono::system_clock::duration current_time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
                    auto current_time_msec = current_time.count() * std::chrono::system_clock::period::num / std::chrono::system_clock::period::den;
                    response.SetTime(current_time_msec);
                    response.SetEventID(event);
                    response.SetModuleID(m_modID);

                    // perform necessary operations and call event
                    if(m_eventCallback.find(event) != m_eventCallback.end()) {
                        m_eventCallback[event](*msg, response);
                    }
                    
                    // notify all subscribers to the event (if list of subscribers is empty, this is quick)
                    // std::cout << "+++Notifying subscribers...";
                    notifySubscribers(event, response);
                    // if a response was requested, then send a response to the requesting process directly
                    if(msg->GetResponseFlag())
                    {
                        // std::cout << "\n+++Sending response...";
                        response.SetTargetModuleID(message.GetModuleID());
                        try {
                            sendMessage(response);
                        }
                        catch(std::exception const& e) {
                            std::cout << e.what() << "\n";
                        }
                    }
                    // std::cout << "\n\n";
                }
            }

            void queryEvents(ModuleID process) {
                EventResponseMessage responseMessage;
                
                responseMessage.SetModuleID(m_modID);
                responseMessage.SetEventID(m_modID*100);
                responseMessage.SetTargetModuleID(process);
                unsigned char* eventBuffer = responseMessage.GetDataBuffer();
                if(m_events.size() * sizeof(EventID) <= responseMessage.GetDataSize())
                for(int i = 0; i < (int)m_events.size(); i++) {
                    // populate the data buffer with the events that are in the supported events list
                    toBuffer(m_events[i], &eventBuffer[sizeof(EventID)*i]);
                }

                try {
                    sendMessage(responseMessage);
                }
                catch(std::exception const& e) {
                    std::cout << e.what() << "\n";
                }
            }

            void notifySubscribers(EventID event, IPCMessage& message) {
                // for every subscriber to the event, modify the response message to  match the process ID and then send it out
                for(std::vector<ModuleID>::iterator it = m_eventSubscribers[event].begin(); it < m_eventSubscribers[event].end(); it++) {
                    // std::cout << *it << "\n";
                    message.SetTargetModuleID(*it);
                    try {
                        sendMessage(message);
                    }
                    catch(std::exception const& e) {
                        std::cout << e.what() << "\n";
                    }
                }
            }
        public:
            EventChannel(ModuleID mID): GenericChannel<EventRequestMessage,EventResponseMessage>(mID, "Event Channel") { }

            void addEventCallback(EventID event, std::function<void(EventRequestMessage&, EventResponseMessage&)> function, bool isRequestEvent = true) {
                if(isRequestEvent) {
                    // if the event isn't already in the list of supported events, add it to the list
                    if(std::find(m_events.begin(), m_events.end(), event) == m_events.end()) {
                        // std::cout << "Adding event " << event << "...\n";
                        m_events.push_back(event);

                        // // set the function as the event's callback
                        // m_eventCallback[event] = function;
                    }
                }
                // add the function to the event's callbacks
                // std::cout << "Adding callback for event " << event << "\n";
                m_eventCallback[event] = function;
            }
        };
    }
}