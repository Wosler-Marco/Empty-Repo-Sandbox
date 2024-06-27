#pragma once

#define US_MAX_BUFFER_SIZE 32

#include <iostream>
#include <iomanip>
#include <cstring>
#include <stdint.h>
#include "SerializationInterface.hpp"
#include "CommonUtilities.hpp"

typedef int32_t InputKeyID;
typedef int8_t USFieldID;

namespace wosler {
namespace utilities {

    enum class USMessageType {
        USSystemControl = 1,
        USParam,
        USScreenControl,
        USSpecialKey
    };

    enum class USControlType {
        Trigger,
        Step,
        Absolute
    };

    enum class USSystemControl {
        Freeze = 1,
        ScanTypeCW,
        ScanTypeB,
        ScanTypeC,
        ScanTypeP,
        ScanTypeM,
        ScanTypeTDI,
        ScanTypePW,
        Cineloop,
        CursorMode,
        MeasurementMode,
        CaliperMode,
        AnnotationMode,
        BodyMarksMode,
        FocalZones,
        SingleImage,
        DualImage,
        QuadImage,
        ClearMeasurements,
        SaveImage,
        PowerDevice,
        PowerBeam,
        ProgrammableKey1,
        ProgrammableKey2,
        ProgrammableKey3,
    };

    enum class USParam {
        Depth = 1,
        TGC,
        ZoomTrue,
        ZoomDigital,
        Angle,
        FocusPos,
        Gain,
        Frequency,
        Contrast,
        Baseline,
        Scale
    };

    enum class USScreenControl {
        Mouse = 1,
        Click,
        AlphaKey,
        SysKey
    };

    enum {
        USIncrease = 1,
        USDecrease
    };

    enum {
        LeftClickPress = 1,
        LeftClickRelease,
        RightClickPress,
        RightClickRelease
    };

    class USControlMessage : public AbstractSerializeableObject {
    private:
        struct ultrasound_data_t
        {
            int8_t messageType = int8_t(USMessageType::USSystemControl);
            int8_t controlType = int8_t(USControlType::Trigger);
            USFieldID field = USFieldID(USSystemControl::Freeze);
            int8_t value[US_MAX_BUFFER_SIZE] = {};
            uint8_t valueSize = 0;
            uint16_t counter;
        } m_message;
        static uint16_t counter;    // counter auto increases with each message created 

    public:
        USControlMessage() : AbstractSerializeableObject(1) {
            counter++;
            m_message.counter = counter;
            std::fill_n(m_message.value, US_MAX_BUFFER_SIZE, 0);
        }
        ~USControlMessage() = default;

        USControlMessage& operator=(const USControlMessage& message) {
            if(this != &message) {
                m_message = message.GetMessage();
            }
            return *this;
        }

        bool operator<(const USControlMessage& other) const {
            return (GetCounter() < other.GetCounter());
        }

        bool operator>(const USControlMessage& other) const {
            return (GetCounter() > other.GetCounter());
        }

        void PrintMessage() {
            std::cout << "\nUltrasound Control Message----------------------------\n";
            std::cout << "Message Type: " << (int)m_message.messageType << "\n";
            std::cout << "Control Type: " << (int)m_message.controlType << "\n";
            std::cout << "Field: " << (int)m_message.field << "\n";
            std::cout << "Value: ";
            for(int i = 0; i < US_MAX_BUFFER_SIZE; i++) {
                std::cout << std::setw(2) << std::hex << (unsigned int)m_message.value[i] << " ";
            }
            std::cout << "\nCounter: " << std::dec << m_message.counter << "\n\n";
        }

        // Getters
        ultrasound_data_t GetMessage() const {
            return m_message;
        }
        int8_t GetMessageType() const {
            return m_message.messageType;
        }
        int8_t GetControlType() const {
            return m_message.controlType;
        }
        USFieldID GetField() const {
            return m_message.field;
        }
        size_t GetValue(void* buffer, size_t buff_size) const {
            if (buff_size >= m_message.valueSize && buffer)
            {
                // TODO: There is a reason we use memcpy here instead of std::copy(...) will write more to elaborate later
                std::memcpy(buffer, &(m_message.value), m_message.valueSize);
                return sizeof(m_message.value);
            }
            return 0;
        }
        size_t GetValueSize() const {
            return m_message.valueSize;
        }
        uint16_t GetCounter() const {
            return m_message.counter;
        }
        size_t GetMessageSize() const {
            return sizeof(m_message);
        }

        // Setters
        void SetMessageType(int8_t messageType) {
            m_message.messageType = messageType;
        }
        void SetControlType(int8_t controlType) {
            m_message.controlType = controlType;
        }
        void SetField(USFieldID fieldID) {
            m_message.field = fieldID;
        }

        template<typename dataType>
        bool SetValue(dataType val) {
            std::fill_n(m_message.value, US_MAX_BUFFER_SIZE, 0);
            // TODO: There is a reason we use memcpy here instead of std::copy(...) will write more to elaborate later
            std::memcpy(&(m_message.value), &val, sizeof(val));
            m_message.valueSize = (uint8_t)sizeof(val);
            return true;
        }

        // overload for buffers
        bool SetValue(uint8_t* val, size_t buff_size=0) {
            if (buff_size <= sizeof(m_message.value) && val)
            {
                std::fill_n(m_message.value, US_MAX_BUFFER_SIZE, 0);
                // TODO: There is a reason we use memcpy here instead of std::copy(...) will write more to elaborate later
                std::memcpy(&(m_message.value), val, buff_size);
                m_message.valueSize = (uint8_t)buff_size;
                return true;
            }
            return false;
        }

        // Serialization Functions
        virtual size_t Serialize(void *raw_buffer, size_t buffer_size_bytes) override
        {
            if (buffer_size_bytes >= sizeof(m_message) && raw_buffer)
            {
                // TODO: There is a reason we use memcpy here instead of std::copy(...) will write more to elaborate later
                std::memcpy(raw_buffer, &m_message, sizeof(m_message));
                return sizeof(m_message);
            }
            return 0;
        }

        virtual bool Deserialize(const void *raw_buffer, size_t buffer_size_bytes) override
        {
            if (buffer_size_bytes >= sizeof(m_message) && raw_buffer)
            {
                // TODO: There is a reason we use memcpy here instead of std::copy(...) will write more to elaborate later
                std::memcpy(&m_message , raw_buffer, sizeof(m_message));
                return true;
            }
            return false;
        }
    };

    // Initialize the static counter member outside the struct definition
    uint16_t USControlMessage::counter = 0;

    template<typename dataType>
    Error createUSMessage(USControlMessage& msg,
                            USFieldID field,
                            USMessageType msgType,
                            USControlType controlType,
                            dataType data) {
        msg.SetMessageType(int8_t(msgType));
        msg.SetControlType(int8_t(controlType));
        msg.SetField(field);
        msg.SetValue(data);
        return Error::SUCCESS;
    }

    // overloaded createUSMessage to allow for buffer data (i.e. TGC)
    Error createUSMessage(USControlMessage& msg,
                            USFieldID field,
                            USMessageType msgType,
                            USControlType controlType,
                            uint8_t* data,
                            size_t dataSize=0) {
        msg.SetMessageType(int8_t(msgType));
        msg.SetControlType(int8_t(controlType));
        msg.SetField(field);
        msg.SetValue(data, dataSize);
        return Error::SUCCESS;
    }

} // namespace utilities
} // namespace wosler