/**
 * @file IPCMessage.hpp
 * @author Matthew Da Silva (matthew.dasilva@wosler.ca)
 * @brief 
 * @version 0.1
 * @date 2023-02-24
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#pragma once

#include "CommonUtilities.hpp"
#include "SerializationInterface.hpp"
#include <cstring>

#define IPC_MAX_BUFFER_SIZE_BYTES 512
#define EVENT_REQUEST_DATA_SIZE 256
#define EVENT_RESPONSE_DATA_SIZE 64
#define EVENT_RESPONSE_ERR_MSG_SIZE 64

#define PROMPT_MAX_DESC_LEN 96
#define PROMPT_MAX_PATH_LEN 64
#define PROMPT_MAX_OPTION_LEN 32
#define PROMPT_MAX_OPTION_NUM 2

namespace wosler {
    namespace utilities {
        enum MessageType {
            MSG_TYPE_Request,
            MSG_TYPE_Response
        };

        struct metadata_t
        {
            int8_t msg_type;
            int64_t time = 0;
            //Sender data
            ModuleID moduleID = 0;
            //Target data
            ModuleID targetModuleID = 0;
        }; 

        // SonoStation GUI and Input Device Specific Structs
        struct Option
        {
            char text[PROMPT_MAX_OPTION_LEN] = "";
            size_t sizeofText = 0;
        };

        struct Prompt
        {
            char desc[PROMPT_MAX_DESC_LEN] = "";
            size_t sizeofDesc = 0;
            char assetPath[PROMPT_MAX_PATH_LEN] = "";
            size_t sizeofPath = 0;
            Option options[PROMPT_MAX_OPTION_NUM];
            size_t numOptions = 0;
        };

        // Motion Control and SonoStation GUI Specific Enums
        // Drag and Drop Status
        enum class DNDStatus {
            Dragging, // Input Device in control of Robot
            Dropped, // Input Device uncoupled from Robot motion
            AttemptingPickup // Input Device matching orientation of Robot so it can start Dragging again
        };


        class IPCMessage : public AbstractSerializeableObject {
        public:
            IPCMessage() : AbstractSerializeableObject(1) {}
            ~IPCMessage() = default;

            // Helpers
            virtual void printMessage() = 0;

            // Getters
            virtual MessageType GetMessageType() = 0;
            virtual int64_t GetTime() = 0;
            virtual ModuleID GetModuleID() = 0;
            virtual ModuleID GetTargetModuleID() = 0;
            virtual size_t GetMessageSize() = 0;

            // Setters
            virtual void SetTime(int64_t time) = 0;
            virtual void SetModuleID(ModuleID id) = 0;
            virtual void SetTargetModuleID(ModuleID id) = 0;

            // Serialization Functions
            virtual size_t Serialize(void *raw_buffer, size_t buffer_size_bytes) = 0;
            virtual bool Deserialize(const void *raw_buffer, size_t buffer_size_bytes) = 0;
        };

        class EventRequestMessage : public IPCMessage {
        private:
            struct request_data_t
            {
                metadata_t mdata;
                FunctionID functionID = 0;
                EventID eventID = 0;
                uint16_t dataBufferSize = IPC_MAX_BUFFER_SIZE_BYTES;
                unsigned char data[IPC_MAX_BUFFER_SIZE_BYTES] = {0};
                bool responseReq = false;
            } request;
        public:
            EventRequestMessage() : IPCMessage() {
                request.dataBufferSize = std::min(EVENT_REQUEST_DATA_SIZE,IPC_MAX_BUFFER_SIZE_BYTES);
                request.mdata.msg_type = MSG_TYPE_Request;
            }
            // EventRequestMessage(IPCMessage m) : IPCMessage() {
            //     request.msg.time = m.GetTime();
            //     request.msg.moduleID = m.GetModuleID();
            //     request.msg.functionID = m.GetFunctionID();
            //     request.msg.eventID = m.GetEventID();
            //     request.msg.dataBufferSize = std::min(EVENT_REQUEST_DATA_SIZE,IPC_MAX_BUFFER_SIZE_BYTES);
            //     memset(request.msg.data,0,sizeof(request.msg.data));
            //     std::memcpy(request.msg.data, m.GetDataBuffer(), m.GetDataSize());
            // }
            ~EventRequestMessage() = default;

            // Helpers
            virtual void printMessage() {
                std::cout << "\nMessage Contents -------------------------\n";
                std::cout << "Message Type: Request\n";
                std::cout << "Time: " << request.mdata.time << "\n";
                std::cout << "Sender Process ID: " << request.mdata.moduleID << "\n";
                std::cout << "Target Process ID: " << request.mdata.targetModuleID << "\n";
                std::cout << "Function ID: " << request.functionID << "\n";
                std::cout << "Event ID: " << request.eventID << "\n";
                std::cout << "Data: " << std::hex;
                for(int i = 0; i < request.dataBufferSize-4; i++) {
                    if((request.data[i]|request.data[i+1]|request.data[i+2]|request.data[i+3]) == 0) {
                        std::cout << "EOF";
                        break;
                    }
                    std::cout << (int)request.data[i] << " ";
                }
                std::cout << std::dec << "\n";
                std::cout << "Response Flag: " << request.responseReq << "\n\n";
            }

            void printMessage(uint16_t printBufferSize) {
                std::cout << "\nMessage Contents -------------------------\n";
                std::cout << "Message Type: Request\n";
                std::cout << "Time: " << request.mdata.time << "\n";
                std::cout << "Sender Process ID: " << request.mdata.moduleID << "\n";
                std::cout << "Target Process ID: " << request.mdata.targetModuleID << "\n";
                std::cout << "Function ID: " << request.functionID << "\n";
                std::cout << "Event ID: " << request.eventID << "\n";
                std::cout << "Data: " << std::hex;
                uint16_t size = (printBufferSize < request.dataBufferSize-4) ? printBufferSize : request.dataBufferSize-4;
                for(int i = 0; i < size; i++) {
                    std::cout << (int)request.data[i] << " ";
                }
                std::cout << std::dec << "\n";
                std::cout << "Response Flag: " << request.responseReq << "\n\n";
            }

            // Getters
            MessageType GetMessageType() {
                return (MessageType)request.mdata.msg_type;
            }
            
            int64_t GetTime() {
                return request.mdata.time;
            }

            ModuleID GetModuleID() {
                return request.mdata.moduleID;
            }

            ModuleID GetTargetModuleID() {
                return request.mdata.targetModuleID;
            }

            FunctionID GetFunctionID() {
                return request.functionID;
            }

            EventID GetEventID() {
                return request.eventID;
            }

            const unsigned char* GetConstDataBuffer() {
                return request.data;
            }

            unsigned char* GetDataBuffer() {
                return request.data;
            }

            bool GetResponseFlag() {
                return request.responseReq;
            }

            size_t GetDataSize() {
                return request.dataBufferSize;
            }

            size_t GetMessageSize() {
                return sizeof(request);
            }

            // Setters
            void SetTime(int64_t time) {
                request.mdata.time = time;
            }

            void SetModuleID(ModuleID id) {
                request.mdata.moduleID = id;
            }

            void SetModuleID(Module id) {
                request.mdata.moduleID = (ModuleID) id;
            }

            void SetTargetModuleID(ModuleID id) {
                request.mdata.targetModuleID = id;
            }

            void SetTargetModuleID(Module id) {
                request.mdata.targetModuleID = (ModuleID) id;
            }

            void SetFunctionID(FunctionID id) {
                request.functionID = id;
            }

            void SetEventID(EventID id) {
                request.eventID = id;
                request.mdata.targetModuleID = (ModuleID)(id/100);
            }

            void SetDataBuffer(const char *raw_buffer, size_t buffer_len) {
                if (buffer_len <= request.dataBufferSize and raw_buffer)
                {
                    std::memset(request.data,0,sizeof(request.data));
                    std::memcpy(request.data, raw_buffer, buffer_len);
                }
                else
                {
                    std::cout << "\nCannot set data! Source buffer is too large...\n";
                }
            }

            void SetResponseFlag(bool flag) {
                request.responseReq = flag;
            }

            // Serialization Functions
            size_t Serialize(void *raw_buffer, size_t buffer_size_bytes) override
            {
                if (buffer_size_bytes >= sizeof(request) and raw_buffer)
                {
                    // TODO: There is a reason we use memcpy here instead of std::copy(...) will write more to elaborate later
                    std::memcpy(raw_buffer, &request, sizeof(request));
                    return sizeof(request);
                }
                return 0;
            }

            bool Deserialize(const void *raw_buffer, size_t buffer_size_bytes) override
            {
                if (buffer_size_bytes >= sizeof(request) and raw_buffer)
                {
                    // TODO: There is a reason we use memcpy here instead of std::copy(...) will write more to elaborate later
                    std::memcpy(&request , raw_buffer, sizeof(request));
                    return true;
                }
                return false;
            }
        };

        class EventResponseMessage : public IPCMessage {
        private:
            struct response_data_t
            {
                metadata_t mdata;
                EventID eventID = 0;
                uint16_t dataBufferSize = IPC_MAX_BUFFER_SIZE_BYTES;
                unsigned char data[IPC_MAX_BUFFER_SIZE_BYTES] = {0};
                ErrorCode errorCode = 0;
                char errorMsg[EVENT_RESPONSE_ERR_MSG_SIZE] = "";
            } response;
        public:
            EventResponseMessage() : IPCMessage() {
                response.dataBufferSize = std::min(EVENT_RESPONSE_DATA_SIZE,IPC_MAX_BUFFER_SIZE_BYTES);
                response.mdata.msg_type = MSG_TYPE_Response;
            }
            ~EventResponseMessage() = default;

            // Helpers
            void printMessage() {
                std::cout << "\nMessage Contents -------------------------\n";
                std::cout << "Message Type: Response\n";
                std::cout << "Time: " << response.mdata.time << "\n";
                std::cout << "Sender Process ID: " << response.mdata.moduleID << "\n";
                std::cout << "Target Process ID: " << response.mdata.targetModuleID << "\n";
                std::cout << "Event ID: " << response.eventID << "\n";
                std::cout << "Data: " << std::hex;
                for(int i = 0; i < response.dataBufferSize-4; i++) {
                    if((response.data[i]|response.data[i+1]|response.data[i+2]|response.data[i+3]) == 0) {
                        std::cout << "EOF";
                        break;
                    }
                    std::cout << (int)response.data[i] << " ";
                }
                std::cout << std::dec << "\n";
                std::cout << "Error Code: " << response.errorCode << "\n";
                std::cout << "Error Message: ";
                for(int i = 0; i < EVENT_RESPONSE_ERR_MSG_SIZE; i++) {
                    if(response.errorMsg[i] == '\0') {
                        std::cout << "EOF";
                        break;
                    }
                    std::cout << response.errorMsg[i];
                }
                std::cout << "\n\n";
            }

            // Getters
            MessageType GetMessageType() {
                return (MessageType)response.mdata.msg_type;
            }
            
            int64_t GetTime() {
                return response.mdata.time;
            }

            ModuleID GetModuleID() {
                return response.mdata.moduleID;
            }

            ModuleID GetTargetModuleID() {
                return response.mdata.targetModuleID;
            }

            EventID GetEventID() {
                return response.eventID;
            }

            const unsigned char* GetConstDataBuffer() {
                return response.data;
            }

            unsigned char* GetDataBuffer() {
                return response.data;
            }

            size_t GetDataSize() {
                return response.dataBufferSize;
            }

            size_t GetMessageSize() {
                return sizeof(response);
            }

            ErrorCode GetErrorCode() {
                return response.errorCode;
            }

            const char* GetConstErrorMessage() {
                return response.errorMsg;
            }

            char* GetErrorMessage() {
                return response.errorMsg;
            }

            // Setters
            void SetTime(int64_t time) {
                response.mdata.time = time;
            }

            void SetModuleID(ModuleID id) {
                response.mdata.moduleID = id;
            }

            void SetModuleID(Module id) {
                response.mdata.moduleID = (ModuleID) id;
            }

            void SetTargetModuleID(ModuleID id) {
                response.mdata.targetModuleID = id;
            }

            void SetTargetModuleID(Module id) {
                response.mdata.targetModuleID = (ModuleID) id;
            }

            void SetEventID(EventID id) {
                response.eventID = id;
                response.mdata.targetModuleID = (ModuleID)(id/100);
            }

            void SetDataBuffer(const char *raw_buffer, size_t buffer_len) {
                if (buffer_len <= response.dataBufferSize and raw_buffer)
                {
                    memset(response.data,0,sizeof(response.data));
                    std::memcpy(response.data, raw_buffer, buffer_len);
                }
                else
                {
                    std::cout << "\nCannot set data! Source buffer is too large...\n";
                }
            }

            void SetErrorCode(ErrorCode err) {
                response.errorCode = err;
            }

            void SetErrorMessage(const char* error_msg, size_t msg_len) {
                if (msg_len <= EVENT_RESPONSE_ERR_MSG_SIZE and error_msg)
                {
                    #ifdef _WIN32
                    strcpy_s(response.errorMsg,EVENT_RESPONSE_ERR_MSG_SIZE,"");
                    #else
                    strcpy(response.errorMsg,"");
                    #endif
                    std::memcpy(response.errorMsg, error_msg, msg_len);
                }
                else
                {
                    std::cout << "\nCannot set message! Source buffer is too large...\n";
                }
            }

            // Serialization Functions
            size_t Serialize(void *raw_buffer, size_t buffer_size_bytes) override
            {
                if (buffer_size_bytes >= sizeof(response) and raw_buffer)
                {
                    // TODO: There is a reason we use memcpy here instead of std::copy(...) will write more to elaborate later
                    std::memcpy(raw_buffer, &response, sizeof(response));
                    return sizeof(response);
                }
                return 0;
            }

            bool Deserialize(const void *raw_buffer, size_t buffer_size_bytes) override
            {
                if (buffer_size_bytes >= sizeof(response) and raw_buffer)
                {
                    // TODO: There is a reason we use memcpy here instead of std::copy(...) will write more to elaborate later
                    std::memcpy(&response , raw_buffer, sizeof(response));
                    return true;
                }
                return false;
            }
        };

        class StateRequestMessage : public IPCMessage {
        private:
            struct request_data_t
            {
                metadata_t mdata;
                StateID stateReq;
            } request;
        public:
            StateRequestMessage() : IPCMessage() {
                request.mdata.msg_type = MSG_TYPE_Request;
            }
            ~StateRequestMessage() = default;

            // Helpers
            virtual void printMessage() {
                std::cout << "\nMessage Contents -------------------------\n";
                std::cout << "Message Type: Request\n";
                std::cout << "Time: " << request.mdata.time << "\n";
                std::cout << "Sender Process ID: " << request.mdata.moduleID << "\n";
                std::cout << "Target Process ID: " << request.mdata.targetModuleID << "\n";
                std::cout << "Requested state: " << request.stateReq << "\n";
            }

            // Getters
            MessageType GetMessageType() {
                return (MessageType)request.mdata.msg_type;
            }
            
            int64_t GetTime() {
                return request.mdata.time;
            }

            ModuleID GetModuleID() {
                return request.mdata.moduleID;
            }

            ModuleID GetTargetModuleID() {
                return request.mdata.targetModuleID;
            }

            StateID GetReqStateID() {
                return request.stateReq;
            }

            size_t GetMessageSize() {
                return sizeof(request);
            }

            // Setters
            void SetTime(int64_t time) {
                request.mdata.time = time;
            }

            void SetModuleID(ModuleID id) {
                request.mdata.moduleID = id;
            }

            void SetModuleID(Module id) {
                request.mdata.moduleID = (ModuleID) id;
            }

            void SetTargetModuleID(ModuleID id) {
                request.mdata.targetModuleID = id;
            }

            void SetTargetModuleID(Module id) {
                request.mdata.targetModuleID = (ModuleID) id;
            }

            void SetReqStateID(StateID id) {
                request.stateReq = id;
            }

            void SetReqStateID(State id) {
                request.stateReq = (StateID) id;
            }

            // Serialization Functions
            size_t Serialize(void *raw_buffer, size_t buffer_size_bytes) override
            {
                if (buffer_size_bytes >= sizeof(request) and raw_buffer)
                {
                    // TODO: There is a reason we use memcpy here instead of std::copy(...) will write more to elaborate later
                    std::memcpy(raw_buffer, &request, sizeof(request));
                    return sizeof(request);
                }
                return 0;
            }

            bool Deserialize(const void *raw_buffer, size_t buffer_size_bytes) override
            {
                if (buffer_size_bytes >= sizeof(request) and raw_buffer)
                {
                    // TODO: There is a reason we use memcpy here instead of std::copy(...) will write more to elaborate later
                    std::memcpy(&request , raw_buffer, sizeof(request));
                    return true;
                }
                return false;
            }
        };

        class StateResponseMessage : public IPCMessage {
        private:
            struct response_data_t
            {
                metadata_t mdata;
                StateID stateCurr;
                ErrorCode errorCode = 0;
                char errorMsg[EVENT_RESPONSE_ERR_MSG_SIZE] = "";
            } response;
        public:
            StateResponseMessage() : IPCMessage() {
                response.mdata.msg_type = MSG_TYPE_Response;
            }
            ~StateResponseMessage() = default;

            // Helpers
            void printMessage() {
                std::cout << "\nMessage Contents -------------------------\n";
                std::cout << "Message Type: Response\n";
                std::cout << "Time: " << response.mdata.time << "\n";
                std::cout << "Sender Process ID: " << response.mdata.moduleID << "\n";
                std::cout << "Target Process ID: " << response.mdata.targetModuleID << "\n";
                std::cout << "Current state: " << (int)response.stateCurr << "\n";
                std::cout << "Error Code: " << response.errorCode << "\n";
                std::cout << "Error Message: ";
                for(int i = 0; i < EVENT_RESPONSE_ERR_MSG_SIZE; i++) {
                    if(response.errorMsg[i] == '\0') {
                        std::cout << "EOF";
                        break;
                    }
                    std::cout << response.errorMsg[i];
                }
                std::cout << "\n\n";
            }

            // Getters
            MessageType GetMessageType() {
                return (MessageType)response.mdata.msg_type;
            }
            
            int64_t GetTime() {
                return response.mdata.time;
            }

            ModuleID GetModuleID() {
                return response.mdata.moduleID;
            }

            ModuleID GetTargetModuleID() {
                return response.mdata.targetModuleID;
            }

            StateID GetCurrStateID() {
                return response.stateCurr;
            }

            size_t GetMessageSize() {
                return sizeof(response);
            }

            ErrorCode GetErrorCode() {
                return response.errorCode;
            }

            const char* GetConstErrorMessage() {
                return response.errorMsg;
            }

            char* GetErrorMessage() {
                return response.errorMsg;
            }

            // Setters
            void SetTime(int64_t time) {
                response.mdata.time = time;
            }

            void SetModuleID(ModuleID id) {
                response.mdata.moduleID = id;
            }

            void SetModuleID(Module id) {
                response.mdata.moduleID = (ModuleID) id;
            }

            void SetTargetModuleID(ModuleID id) {
                response.mdata.targetModuleID = id;
            }

            void SetTargetModuleID(Module id) {
                response.mdata.targetModuleID = (ModuleID) id;
            }

            void SetCurrStateID(StateID id) {
                response.stateCurr = id;
            }

            void SetCurrStateID(State id) {
                response.stateCurr = (StateID) id;
            }

            void SetErrorCode(ErrorCode err) {
                response.errorCode = err;
            }

            void SetErrorMessage(const char* error_msg, size_t msg_len) {
                if (msg_len <= EVENT_RESPONSE_ERR_MSG_SIZE and error_msg)
                {
                    #ifdef _WIN32
                    strcpy_s(response.errorMsg,EVENT_RESPONSE_ERR_MSG_SIZE,"");
                    #else
                    strcpy(response.errorMsg,"");
                    #endif
                    std::memcpy(response.errorMsg, error_msg, msg_len);
                }
                else
                {
                    std::cout << "\nCannot set message! Source buffer is too large...\n";
                }
            }

            // Serialization Functions
            size_t Serialize(void *raw_buffer, size_t buffer_size_bytes) override
            {
                if (buffer_size_bytes >= sizeof(response) and raw_buffer)
                {
                    // TODO: There is a reason we use memcpy here instead of std::copy(...) will write more to elaborate later
                    std::memcpy(raw_buffer, &response, sizeof(response));
                    return sizeof(response);
                }
                return 0;
            }

            bool Deserialize(const void *raw_buffer, size_t buffer_size_bytes) override
            {
                if (buffer_size_bytes >= sizeof(response) and raw_buffer)
                {
                    // TODO: There is a reason we use memcpy here instead of std::copy(...) will write more to elaborate later
                    std::memcpy(&response , raw_buffer, sizeof(response));
                    return true;
                }
                return false;
            }
        };
    }
}