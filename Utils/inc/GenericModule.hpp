/**
 * @file GenericModule.hpp
 * @author Matthew Da Silva (matthew.dasilva@wosler.ca)
 * @brief 
 * @version 0.1
 * @date 2023-02-24
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#pragma once

#include "EventChannel.hpp"
#include "StateChannel.hpp"

namespace wosler {
    namespace utilities {
        class GenericModule {
        protected:
            std::unique_ptr<EventChannel> m_eventChannel;
            std::unique_ptr<StateChannel> m_stateChannel;
            ModuleID m_modID;
            std::atomic<State> m_state{State::Standby};
            ErrorCode stateErrCode = 0;
            char stateErrMsg[EVENT_RESPONSE_ERR_MSG_SIZE] = "";

            /**
             * @brief Generic method to post data for subscribed modules to read
             * 
             * @param paramMsg The buffer containing a serialized data
             * @param returnMsg The buffer to contain a copy of the serialized data for deserialization in subscriber callbacks
             */
            void postDataForSub(wosler::utilities::EventRequestMessage& paramMsg, wosler::utilities::EventResponseMessage& returnMsg) {
                const unsigned char* dataBuffer = paramMsg.GetConstDataBuffer();
                std::memcpy(returnMsg.GetDataBuffer(), dataBuffer, IPC_MAX_BUFFER_SIZE_BYTES);
            }

            /**
             * @brief Pure virtual function to initialize the module (module independent functions: hardware confirmation, diagnostics, etc.)
             * 
             * @return The error status as a result of performing the initialization functions
             */
            virtual Error stateInit() = 0;

            /**
             * @brief Pure virtual function to setup the module (module dependent functions: connect to other modules, create/open sockets, connect to external hardware, etc.)
             * 
             * @return The error status as a result of performing the setup functions
             */
            virtual Error stateSetup() = 0;

            /**
             * @brief Pure virtual function to start running the module (start any normal operations/continuous periodic code)
             * 
             * @return The error status as a result of performing the running functions
             */
            virtual Error stateRunning() = 0;

            /**
             * @brief Pure virtual function to put the module into an error state (diminish functionality, stop/pause operations, etc.)
             * 
             * @return The error status as a result of performing the error functions
             */
            virtual Error stateError() = 0;

            /**
             * @brief Pure virtual function to shutdown the module (close all module processes)
             * 
             * @return The error status as a result of performing the shutdown functions
             */
            virtual Error stateShutdown() = 0;

            void onStateChange(StateRequestMessage& reqMsg, StateResponseMessage& returnMsg) {
                StateID stateReq = reqMsg.GetReqStateID();
                // std::cout << "Received request from server to change state to " << stateReq << "\n";

                Error err;
                stateErrCode = 0;
                std::strncpy(stateErrMsg, "", sizeof(stateErrMsg));

                switch(stateReq) {
                case (StateID)State::Initializing:
                    if(m_state == State::Standby) {
                        err = stateInit();
                        if(err == Error::SUCCESS) {
                            m_state = State::Initializing;
                        }
                    }
                    else {
                        stateErrCode = 0x000;
                        std::strncpy(stateErrMsg, ("Not in the right state to transition to setup: " + std::to_string((StateID)m_state.load())).c_str(), sizeof(stateErrMsg));
                        err = Error::INVALID_STATE;
                    }
                    break;
                case (StateID)State::Setup:
                    if(m_state == State::Initializing || m_state == State::Error || m_state == State::Shutdown) {
                        err = stateSetup();
                        if(err == Error::SUCCESS) {
                            m_state = State::Setup;
                        }
                    }
                    else{
                        stateErrCode = 0x100;
                        std::strncpy(stateErrMsg, ("Not in the right state to transition to setup: " + std::to_string((StateID)m_state.load())).c_str(), sizeof(stateErrMsg));
                        err = Error::INVALID_STATE;
                    }
                    break;
                case (StateID)State::Running:
                    if(m_state == State::Setup) {
                        err = stateRunning();
                        if(err == Error::SUCCESS) {
                            m_state = State::Running;
                        }
                    }
                    else {
                        stateErrCode = 0x200;
                        std::strncpy(stateErrMsg, ("Not in the right state to transition to running: " + std::to_string((StateID)m_state.load())).c_str(), sizeof(stateErrMsg));
                        err = Error::INVALID_STATE;
                    }
                    break;
                case (StateID)State::Error:
                    err = stateError();
                    if(err == Error::SUCCESS) {
                        m_state = State::Error;
                    }
                    break;
                case (StateID)State::Shutdown:
                    err = stateShutdown();
                    if(err == Error::SUCCESS) {
                        m_state = State::Shutdown;
                    }
                    break;
                }
                if(err != Error::SUCCESS) {
                    std::cout << "Error Type: " << (ErrorCode) err << "\n";
                    std::cout << "Error Code: " << stateErrCode << "\n";
                    std::cout << "Error Message: " << stateErrMsg << "\n";
                    returnMsg.SetErrorCode(stateErrCode);
                    returnMsg.SetErrorMessage(stateErrMsg,sizeof(stateErrMsg));
                    // update heart beat signal with error code
                }
                returnMsg.SetCurrStateID(m_state);
            };
            virtual void onStateResponse(StateRequestMessage& reqMsg, StateResponseMessage& returnMsg) { };

        public:
            GenericModule(ModuleID mID, bool isClient = true) {
                m_modID = mID;
                m_eventChannel = std::make_unique<EventChannel>(m_modID);
                m_stateChannel = std::make_unique<StateChannel>(m_modID, isClient);

                Error error = m_stateChannel->Start();
                if(error != Error::SUCCESS) { throw "Couldn't open state channel: " + std::to_string((int)error); }

                if(isClient) {
                    m_stateChannel->addStateChangeCallback([=](StateRequestMessage& reqMsg, StateResponseMessage& returnMsg) {
                        this->onStateChange(reqMsg, returnMsg);
                    });

                    StateResponseMessage msg;
                    msg.SetModuleID(m_modID);
                    msg.SetTargetModuleID(Module::SYSTEM_CONTROLLER);
                    msg.SetCurrStateID(m_state);
                    try {
                        m_stateChannel->sendMessage(msg);
                    }
                    catch(std::exception const& e) {
                        std::cout << e.what() << "\n";
                    }
                }
                else {
                    m_stateChannel->addStateResponseCallback([=](StateRequestMessage& reqMsg, StateResponseMessage& returnMsg) {
                        this->onStateResponse(reqMsg, returnMsg);
                    });
                }
            }

            ~GenericModule() {
                m_stateChannel->Stop();
            }

            virtual Error Start() {
                return m_eventChannel->Start();
            }

            virtual Error Stop() {
                return m_eventChannel->Stop();
            }
        };
    }
}