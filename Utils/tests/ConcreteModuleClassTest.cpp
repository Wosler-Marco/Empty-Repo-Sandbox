#include "GenericModule.hpp"
#include "IPCMessage.hpp"
#include "ModuleEvents.h"

class ConcreteModule : public wosler::utilities::GenericModule {
private:
    bool haptics = true;

    void setOffset(wosler::utilities::EventRequestMessage& paramMsg, wosler::utilities::EventResponseMessage& returnMsg) {
        std::cout << "Setting offset...\n";
        char data[EVENT_RESPONSE_DATA_SIZE] = "offset...";
        returnMsg.SetDataBuffer(data,sizeof(data));
    }

    void setDemPosForce(wosler::utilities::EventRequestMessage& paramMsg, wosler::utilities::EventResponseMessage& returnMsg) {
        std::cout << "Setting demand position and force...\n";
        returnMsg.SetErrorCode(12);
        char error[EVENT_RESPONSE_ERR_MSG_SIZE] = "Couldn't set position...";
        returnMsg.SetErrorMessage(error,sizeof(error));
    }
    
    void flipControl(wosler::utilities::EventRequestMessage& paramMsg, wosler::utilities::EventResponseMessage& returnMsg) {
        std::cout << "Flipping control...\n";
        char data[EVENT_RESPONSE_DATA_SIZE] = "flipped control...";
        returnMsg.SetDataBuffer(data,sizeof(data));
    }

    void toggleHaptics(wosler::utilities::EventRequestMessage& paramMsg, wosler::utilities::EventResponseMessage& returnMsg) {
        std::cout << "Toggling haptics...\n";
    }

    void setScale(wosler::utilities::EventRequestMessage& paramMsg, wosler::utilities::EventResponseMessage& returnMsg) {
        std::cout << "Setting scale...\n";
    }

    wosler::utilities::Error stateInit() override {
        return wosler::utilities::Error::SUCCESS;
    }

    wosler::utilities::Error stateSetup() override {
        return wosler::utilities::Error::SUCCESS;
    }

    wosler::utilities::Error stateRunning() override {
        return wosler::utilities::Error::SUCCESS;
    }

    wosler::utilities::Error stateError() override {
        return wosler::utilities::Error::SUCCESS;
    }

    wosler::utilities::Error stateShutdown() override {
        return wosler::utilities::Error::SUCCESS;
    }
    
public:
    ConcreteModule(ModuleID mID) : wosler::utilities::GenericModule(mID) {
        m_eventChannel->addEventCallback(wosler::utilities::MOTION_CONTROL_SetOffset, 
            [=](wosler::utilities::EventRequestMessage& paramMsg, wosler::utilities::EventResponseMessage& returnMsg) {
                this->setOffset(paramMsg, returnMsg);
            });
        m_eventChannel->addEventCallback(wosler::utilities::SONOLINK_PostDesiredPosForce, 
            [=](wosler::utilities::EventRequestMessage& paramMsg, wosler::utilities::EventResponseMessage& returnMsg) {
                this->setDemPosForce(paramMsg, returnMsg);
            });
        m_eventChannel->addEventCallback(wosler::utilities::MOTION_CONTROL_FlipControl, 
            [=](wosler::utilities::EventRequestMessage& paramMsg, wosler::utilities::EventResponseMessage& returnMsg) {
                this->flipControl(paramMsg, returnMsg);
            });
        m_eventChannel->addEventCallback(wosler::utilities::MOTION_CONTROL_ToggleHaptics, 
            [=](wosler::utilities::EventRequestMessage& paramMsg, wosler::utilities::EventResponseMessage& returnMsg) {
                this->toggleHaptics(paramMsg, returnMsg);
            });
        m_eventChannel->addEventCallback(wosler::utilities::MOTION_CONTROL_SetScale, 
            [=](wosler::utilities::EventRequestMessage& paramMsg, wosler::utilities::EventResponseMessage& returnMsg) {
                this->setScale(paramMsg, returnMsg);
            });
    }
};

const std::string USAGE = R"raw_usage(
[MODULE_NUMBER:string]

To test this app:
ConcreteModuleClassTest 0
)raw_usage";

int main(int argc, char **argv) {
    if (argc != 2)
    {
        std::cout << argv[0];
        std::cout << USAGE << "\n";
        return 0;
    }

    ConcreteModule module(std::stoi(argv[1]));
    module.Start();
    std::this_thread::sleep_for(std::chrono::seconds(360));
    std::cout << "Concrete module test stopping Module...\n";
    module.Stop();
    return 0;
}