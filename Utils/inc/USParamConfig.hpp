#ifndef USPARAMCONFIG_H_
#define USPARAMCONFIG_H_

#include <cstdint>
#include <unordered_map>
#include <vector>

#include "UltrasoundMessage.hpp"

namespace wosler {
namespace utilities {

enum class typeSpec {
    UInt8,
    Float
};

struct USParamInfo{
    USControlType controlType = USControlType::Step;
    int64_t minVal = 0;
    int64_t maxVal = 0;
    int64_t modifier = 0;
    typeSpec type = typeSpec::UInt8;
};

template<typename dataType>
uint64_t encodeToUint64(dataType value) {
    static_assert(sizeof(dataType) <= sizeof(uint64_t), "dataType size must be less than or equal to uint64_t size");
    
    uint64_t result;
    std::memcpy(&result, &value, sizeof(dataType));
    return result;
}

template<typename dataType>
dataType decodeFromUint64(uint64_t encodedValue) {
    static_assert(sizeof(dataType) <= sizeof(uint64_t), "dataType size must be less than or equal to uint64_t size");
    
    dataType result;
    std::memcpy(&result, &encodedValue, sizeof(dataType));
    return result;
}

struct USParamConfig{
    std::unordered_map<USParam,USParamInfo> paramInfo;

    USParamConfig() { }

    USParamConfig(const USParamConfig& copy)
    {
        paramInfo = copy.paramInfo;
    }

    USParamConfig(const std::unordered_map<USParam,USParamInfo>& params)
    {
        paramInfo = params;
    }
};

// Mindray MX8 Ultrasound Config
static const USParamConfig MX8ParamConfig = {{
    {USParam::Depth,{USControlType::Step}},
    {USParam::Angle,{USControlType::Step}},
    {USParam::TGC,{USControlType::Absolute,0,100,10,typeSpec::UInt8}},
    {USParam::ZoomTrue,{USControlType::Step}},
    {USParam::ZoomDigital,{USControlType::Step}},
    {USParam::Angle,{USControlType::Step}},
    {USParam::FocusPos,{USControlType::Step}},
    {USParam::Gain,{USControlType::Step}},
    {USParam::Frequency,{USControlType::Step}},
    {USParam::Contrast,{USControlType::Step}},
    {USParam::Baseline,{USControlType::Step}},
    {USParam::Scale,{USControlType::Step}}
}};

}
}

#endif