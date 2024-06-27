#include "GenericModule.hpp"
#include "IPCMessage.hpp"
#include "ModuleEvents.h"
#include "StateChannel.hpp"
#include "CommonUtilities.hpp"

bool sendRequest;
StateID stateReq = 4;
ModuleID moduleReq = 2;

class InputsRunnable : public wosler::utilities::AsyncRunnable
{
private:
    char command;

public:
    InputsRunnable() : wosler::utilities::AsyncRunnable("Inputs Runnable"){};
    ~InputsRunnable() = default;
    virtual void OnProcess() override
    {
        std::cout << "Enter command: ";
        std::cin >> command;

        if(command == 'q') {
            sendRequest = true;
        }
        else if(command == 'w') {
            std::cout << "Enter a state ID: ";
            std::cin >> stateReq;
        }
        else if(command == 'e') {
            std::cout << "Enter a module ID: ";
            std::cin >> moduleReq;
        }
    }
};

class ConcreteModule : public wosler::utilities::GenericModule, protected wosler::utilities::AsyncRunnable {
private:
    bool m_client;
    uint64_t task_period_ms = 100;

    void OnProcess() override {
        if(!m_client && sendRequest) {
            std::cout << "Sending request to client " << moduleReq << " to change state to " << stateReq << "\n";
            wosler::utilities::StateRequestMessage request;
            request.SetModuleID(m_modID);
            request.SetTargetModuleID(moduleReq);
            request.SetReqStateID(stateReq);
            try {
                m_stateChannel->sendMessage(request);
            }
            catch(std::exception const& e) {
                std::cout << e.what() << "\n";
            }

            sendRequest = false;
        }
    }

    void changeState(wosler::utilities::StateRequestMessage& paramMsg, wosler::utilities::StateResponseMessage& returnMsg) {
        StateID stateReq = paramMsg.GetReqStateID();
        std::cout << "Received request from server to change state to " << stateReq << "\n";
        
        returnMsg.SetModuleID(m_modID);
        returnMsg.SetTargetModuleID(paramMsg.GetModuleID());
        returnMsg.SetCurrStateID((StateID) wosler::utilities::State::Standby);
    }

    void printState(wosler::utilities::StateRequestMessage& paramMsg, wosler::utilities::StateResponseMessage& returnMsg) {
        std::cout << "Module " << returnMsg.GetModuleID() << "'s current state is " << returnMsg.GetCurrStateID() << "\n";
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
    ConcreteModule(ModuleID mID, bool isClient) : wosler::utilities::GenericModule(mID,isClient), wosler::utilities::AsyncRunnable("Motion Control") {
        m_client = isClient;

        if(isClient) {
            m_stateChannel->addStateChangeCallback([=](wosler::utilities::StateRequestMessage& paramMsg, wosler::utilities::StateResponseMessage& returnMsg) {
                                                        this->changeState(paramMsg, returnMsg);
                                                    });
        }
        else {
            m_stateChannel->addStateResponseCallback([=](wosler::utilities::StateRequestMessage& paramMsg, wosler::utilities::StateResponseMessage& returnMsg) {
                                                        this->printState(paramMsg, returnMsg);
                                                    });
        }
    }

    wosler::utilities::Error Start() override {
        std::cout << "Starting\n";
        wosler::utilities::Error error = m_eventChannel->Start();
        if(error != wosler::utilities::Error::SUCCESS) { return error; }
        return RunTaskPeriodic(task_period_ms);
    }

    wosler::utilities::Error Stop() override {
        wosler::utilities::Error error = StopTask();
        if(error != wosler::utilities::Error::SUCCESS) { return error; }
        return m_eventChannel->Stop();
    }
};

const std::string USAGE = R"raw_usage(
[IP_ADDRESS:string] [TARGET_PORT_NUMBER:string]

Default socket port number is 4500

To test this app:
StateChannelTest 1 CLIENT 
)raw_usage";

int main(int argc, char **argv) {
    if (argc != 3)
    {
        std::cout << argv[0];
        std::cout << USAGE << "\n";
        return 0;
    }
    ModuleID modID = std::stoi(argv[1]);
    bool client = false;
    std::string client_or_server = argv[2];
    if(client_or_server.compare("CLIENT") == 0) {
        std::cout << "Client...\n";
        client = true;
    }
    ConcreteModule module(modID, client);
    InputsRunnable inputs;

    module.Start();
    inputs.RunTaskPeriodic(100);

    std::this_thread::sleep_for(std::chrono::seconds(3600));
    std::cout << "State Channel Test Stopping Module...\n";
    module.Stop();
    inputs.StopTask();

    return 0;
}