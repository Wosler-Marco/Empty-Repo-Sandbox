#ifndef IPCCONFIG_H_
#define IPCCONFIG_H_

#include <cstdint>
#include <unordered_map>
#include <vector>

#include "CommonUtilities.hpp"

namespace wosler {
namespace utilities {

struct ChannelSocketParam{
    std::string ip;
    int input_port;
    int output_port;
};

// assumes a single channel will use one IP and two ports for communication
struct ModuleConfig{
    std::unordered_map<std::string,ChannelSocketParam> socketParams;

    ModuleConfig() { }

    ModuleConfig(const ModuleConfig& copy)
    {
        socketParams = copy.socketParams;
    }

    ModuleConfig(const std::unordered_map<std::string,ChannelSocketParam>& params)
    {
        socketParams = params;
    }
};

static const std::unordered_map<ModuleID, ModuleConfig> ModuleConfig_Map = {
    {(ModuleID)Module::SONOLINK_INTERFACE,ModuleConfig({{"Event Channel",{"127.0.0.2",15000,16000}},
                                                        {"State Channel",{"127.0.0.3",15000,16000}}})},

    {(ModuleID)Module::MOTION_CONTROL,ModuleConfig({    {"Event Channel",{"127.0.0.2",15001,16001}},
                                                        {"State Channel",{"127.0.0.3",15001,16001}}})},

    {(ModuleID)Module::INPUT_DEVICE,ModuleConfig({      {"Event Channel",{"127.0.0.2",15002,16002}},
                                                        {"State Channel",{"127.0.0.3",15002,16002}}})},

    {(ModuleID)Module::SYSTEM_CONTROLLER,ModuleConfig({ {"Event Channel",{"127.0.0.2",15003,16003}},
                                                        {"State Channel",{"127.0.0.3",15003,16003}}})},

    {(ModuleID)Module::SAFETY_MONITOR,ModuleConfig({    {"Event Channel",{"127.0.0.2",15004,16004}},
                                                        {"State Channel",{"127.0.0.3",15004,16004}}})},

    {(ModuleID)Module::VIDEO_SERVICE,ModuleConfig({     {"Event Channel",{"127.0.0.2",15005,16005}},
                                                        {"State Channel",{"127.0.0.3",15005,16005}}})},

    {(ModuleID)Module::AUDIO_SERVICE,ModuleConfig({     {"Event Channel",{"127.0.0.2",15006,16006}},
                                                        {"State Channel",{"127.0.0.3",15006,16006}}})},

    {(ModuleID)Module::SONOSTATION_GUI,ModuleConfig({   {"Event Channel",{"127.0.0.2",15007,16007}},
                                                        {"State Channel",{"127.0.0.3",15007,16007}}})},
    
    {(ModuleID)Module::ULTRASOUND_INPUT,ModuleConfig({  {"Event Channel",{"127.0.0.2",15008,16008}},
                                                        {"State Channel",{"127.0.0.3",15008,116008}}})}
};

}
}

#endif