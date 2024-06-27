/**
 * @file CommomUtilities.hpp
 * @author Jai Sood (ankur.jai.sood@gmail.com), Matthew Da Silva (matthew.dasilva@wosler.ca)
 * @brief 
 * @version 0.1
 * @date 2022-04-15
 * 
 * @copyright Copyright (c) 2022 Wosler Inc.
 * 
 */

#pragma once

#include <sstream>
#include <iostream>
// #include <cstdint>

typedef uint16_t EventID;
typedef uint16_t ModuleID;
typedef uint16_t FunctionID;
typedef uint16_t ErrorCode;
typedef uint16_t ChannelID;
typedef uint16_t StateID;

namespace wosler {
namespace utilities {

enum class State {
    Standby = 0,
    Initializing,
    Setup,
    Running,
    Error,
    Shutdown
};

enum class Module {
    SONOLINK_INTERFACE = 1,
    MOTION_CONTROL,
    INPUT_DEVICE,
    SYSTEM_CONTROLLER,
    SAFETY_MONITOR,
    VIDEO_SERVICE,
    AUDIO_SERVICE,
    SONOSTATION_GUI,
    ULTRASOUND_INPUT
};

inline void toBuffer(uint8_t data, unsigned char* destBuffer) {
    destBuffer[0] = data;
}

inline void toBuffer(uint16_t data, unsigned char* destBuffer) {
    destBuffer[1] = data & 0xFF;
    destBuffer[0] = (data >> 4) & 0xFF;
}

inline void fromBuffer(uint8_t& data, const unsigned char* srcBuffer) {
    data = srcBuffer[0];
}

inline void fromBuffer(uint16_t& data, const unsigned char* srcBuffer) {
    data = (srcBuffer[0] << 4) | srcBuffer[1];
}

inline void PrintRawBuffer(const unsigned char* data, const size_t size) {
    #ifdef DEBUG

    std::stringstream ss;
    ss << std::hex;
    for(size_t i = 0; i < size; i++) {
        ss << static_cast<int>(data[i]) << " ";
    }
    ss << "\n";
    std::cout << ss.str();

    #endif
}

enum class Error {
    NONE = 0,
    SUCCESS,
    FAILED,
    ALREADY_OPEN,
    INVALID_STATE,
    BAD_PARAMETER,
    BAD_CONFIGURATION,
    TIMEOUT
};

} // end namespace wosler
} // end namespace utilities
