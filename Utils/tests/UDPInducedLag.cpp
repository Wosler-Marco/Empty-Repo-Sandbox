/**
 * @file SocketTest.cpp
 * @author Jai Sood (ankur.jai.sood@gmail.com)
 * @brief Test cases for socket interface
 * @version 0.1
 * @date 2022-04-15
 * 
 * @copyright Copyright (c) 2022 Wosler
 * 
 */

#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <array>
#include <queue>

#include "AsyncRunnable.hpp"
#include "SocketInterface.hpp"
#include "SerializationInterface.hpp"
#include "Eigen/Dense"
#include "TMatrixPacket.hpp"

const int BUFF_SIZE = 512;
const uint16_t PORT_NUMBER = 5000;

uint64_t delay_time_rob = 200;
uint64_t delay_time_id = 0;

struct PacketTuple {
    std::chrono::time_point<std::chrono::high_resolution_clock> recieveTime;
    std::vector<char> data = std::vector<char>(BUFF_SIZE);
};

std::mutex cout_mut;

std::queue<PacketTuple> sendQueue;
std::queue<PacketTuple> sendQueueRob;

wosler::utilities::TmatrixPacket packet;

class RecieveRunnableRob : public wosler::utilities::AsyncRunnable
{
private:
    uint64_t task_counter{0};

    bool handshake{false};

    std::string ip_address;
    std::string target_ip;

    std::unique_ptr<wosler::utilities::UDPSocketInterface> socketInterface;
    sockaddr_in socket_config;
    sockaddr_in target_config;

    std::vector<char> raw_data_buffer;
    size_t buffer_size_bytes{BUFF_SIZE};

    wosler::utilities::Error error{wosler::utilities::Error::NONE};

    bool first = true;
    int buff = 1;

public:
    RecieveRunnableRob(char **udpParam) : wosler::utilities::AsyncRunnable("Recieve Communications Runnable")
    {
        ip_address = udpParam[0];
        target_ip = udpParam[1];

        socketInterface = std::make_unique<wosler::utilities::UDPSocketInterface>(
            ip_address,
            static_cast<uint16_t>(PORT_NUMBER),
            BUFF_SIZE);

        raw_data_buffer.resize(buffer_size_bytes);

        error = wosler::utilities::Error::NONE;

        socket_config.sin_addr.s_addr = inet_addr(ip_address.data());
        socket_config.sin_family = AF_INET;
        socket_config.sin_port = htons(PORT_NUMBER);
        memset(&(socket_config.sin_zero), 0, 8);

        cout_mut.lock();
        std::cout << "Recieve opening socket to " << ip_address << ":" << PORT_NUMBER << "\n";
        cout_mut.unlock();
        
        if (socketInterface.get())
        {
            error = socketInterface->Open();
        }

        if (error != wosler::utilities::Error::SUCCESS)
        {
            throw std::string("Failed to open socket!");
        }

        cout_mut.lock();
        std::cout << "Binding Socket...\n";
        cout_mut.unlock();

        if (socketInterface.get())
        {
            error = socketInterface->Bind();
        }

        if (error != wosler::utilities::Error::SUCCESS)
        {
            throw std::string("Failed to bind socket!");
        }
    }

    ~RecieveRunnableRob()
    {
        StopTask();
    }

    wosler::utilities::Error StopTask()
    {
        if (socketInterface.get())
        {
            return socketInterface->Close();
        }
        return wosler::utilities::Error::SUCCESS;
    }

    virtual void OnProcess() override
    {
        /*
        Thread 1: Read T_dem from UDP
        1. Handshake with Input Device UDP
        2. Read UDP to get T_dem
        3. Save T_dem to memory
        */
        using namespace std::chrono_literals;

        static sockaddr_in sender_config;
        static size_t sender_config_len = sizeof(sender_config);
        memset(&sender_config, 0, sizeof(sender_config));

        try
        {
            if (socketInterface.get())
            {
                // std::cout << "Rob waiting for receive...\n";
                error = socketInterface->RecvFrom(
                    raw_data_buffer.data(),
                    buff,
                    reinterpret_cast<sockaddr *>(&sender_config),
                    reinterpret_cast<socklen_t *>(&sender_config_len));
            }
            else
            {
                std::cout << "ERROR";
            }

            if(first) {
                first = false;
                // set buffer to read full packet after handshake packet
                buff = packet.GetSize();
            }

            if (error != wosler::utilities::Error::SUCCESS)
            {
                throw "Robot Failed to receive data on socket: " + std::to_string(static_cast<int>(error));
            }
            else
            {
                // std::cout << "Recieved Rob: " << raw_data_buffer.data() << "\n";

                PacketTuple sendTuple = {std::chrono::high_resolution_clock::now(), raw_data_buffer};
                sendQueueRob.push(sendTuple);
            }
        }
        catch (std::string msg)
        {
            cout_mut.lock();
            std::cout << msg << std::endl;
            cout_mut.unlock();

            throw -1;
        }
    }
};

class SendRunnableRob : public wosler::utilities::AsyncRunnable
{
private:
    uint64_t task_counter{0};

    bool handshake{false};

    std::string ip_address;
    std::string target_ip;

    std::unique_ptr<wosler::utilities::UDPSocketInterface> socketInterface;
    sockaddr_in socket_config;
    sockaddr_in target_config;

    std::vector<unsigned char> raw_data_buffer;
    size_t buffer_size_bytes{BUFF_SIZE};

    wosler::utilities::Error error{wosler::utilities::Error::NONE};

    bool first = true;
    int buff = 1;

public:
    SendRunnableRob(char **udpParam) : wosler::utilities::AsyncRunnable("Send Communications Runnable")
    {
        ip_address = udpParam[0];
        target_ip = udpParam[1];

        socketInterface = std::make_unique<wosler::utilities::UDPSocketInterface>(
            ip_address,
            PORT_NUMBER,
            BUFF_SIZE);

        raw_data_buffer.resize(buffer_size_bytes);

        socket_config.sin_addr.s_addr = inet_addr(ip_address.data());
        socket_config.sin_family = AF_INET;
        socket_config.sin_port = htons(PORT_NUMBER);
        memset(&(socket_config.sin_zero), 0, 8);

        target_config.sin_addr.s_addr = inet_addr(target_ip.data());
        target_config.sin_family = AF_INET;
        target_config.sin_port = htons(PORT_NUMBER);
        memset(&(target_config.sin_zero), 0, 8);

        cout_mut.lock();
        std::cout << "Send opening socket to " << ip_address << ":" << PORT_NUMBER << "\n";
        cout_mut.unlock();

        if (socketInterface.get())
        {
            error = socketInterface->Open();
        }
        if (error != wosler::utilities::Error::SUCCESS)
        {
            throw std::string("Failed to open socket!");
        }
    };

    ~SendRunnableRob()
    {
        if (socketInterface.get())
        {
            socketInterface->Close();
        }
        StopTask();
    };

    virtual void OnProcess() override
    {
        /*
        Thread 0: Send T_curr via UDP
        1. Handshake UDP with Input Device
        2. Get robot's T_curr (current position) from memory
        3. Send T_curr through UDP
        */
        using namespace std::chrono_literals;

        size_t target_config_len = sizeof(socket_config);
        
        if(!sendQueueRob.empty() && ((std::chrono::high_resolution_clock::now() - sendQueueRob.front().recieveTime) > std::chrono::milliseconds(delay_time_rob))) {
            try
            {
                PacketTuple pack = sendQueueRob.front();
                sendQueueRob.pop();

                // std::cout << "Sending Rob: " << pack.data.data() << "\n";

                if (socketInterface.get())
                {
                    error = socketInterface->SendTo(
                        pack.data.data(),
                        buff,
                        reinterpret_cast<sockaddr *>(&target_config),
                        reinterpret_cast<socklen_t *>(&target_config_len));
                    
                    if(first) {
                        first = false;
                        buff = packet.GetSize();
                    }
                }

                if (error != wosler::utilities::Error::SUCCESS)
                {
                    throw "Robot Failed to send data on socket: " + std::to_string(static_cast<int>(error));
                }
            }
            catch (std::string msg)
            {
                cout_mut.lock();
                std::cout << msg << std::endl;
                cout_mut.unlock();
                
                throw -1;
            }
        }
    }
};

class RecieveRunnableID : public wosler::utilities::AsyncRunnable
{
private:
    uint64_t task_counter{0};

    bool handshake{false};

    std::string ip_address;
    std::string target_ip;

    std::unique_ptr<wosler::utilities::UDPSocketInterface> socketInterface;
    sockaddr_in socket_config;
    sockaddr_in target_config;

    std::vector<char> raw_data_buffer;
    size_t buffer_size_bytes{BUFF_SIZE};

    wosler::utilities::Error error{wosler::utilities::Error::NONE};

    bool first = true;
    int buff = 1;

public:
    RecieveRunnableID(char **udpParam) : wosler::utilities::AsyncRunnable("Recieve Communications Runnable")
    {
        ip_address = udpParam[0];
        target_ip = udpParam[1];

        socketInterface = std::make_unique<wosler::utilities::UDPSocketInterface>(
            ip_address,
            static_cast<uint16_t>(PORT_NUMBER),
            BUFF_SIZE);

        raw_data_buffer.resize(buffer_size_bytes);

        error = wosler::utilities::Error::NONE;

        socket_config.sin_addr.s_addr = inet_addr(ip_address.data());
        socket_config.sin_family = AF_INET;
        socket_config.sin_port = htons(PORT_NUMBER);
        memset(&(socket_config.sin_zero), 0, 8);

        cout_mut.lock();
        std::cout << "Recieve opening socket to " << ip_address << ":" << PORT_NUMBER << "\n";
        cout_mut.unlock();
        
        if (socketInterface.get())
        {
            error = socketInterface->Open();
        }

        if (error != wosler::utilities::Error::SUCCESS)
        {
            throw std::string("Failed to open socket!");
        }

        cout_mut.lock();
        std::cout << "Binding Socket...\n";
        cout_mut.unlock();

        if (socketInterface.get())
        {
            error = socketInterface->Bind();
        }

        if (error != wosler::utilities::Error::SUCCESS)
        {
            throw std::string("Failed to bind socket!");
        }
    }

    ~RecieveRunnableID()
    {
        StopTask();
    }

    wosler::utilities::Error StopTask()
    {
        if (socketInterface.get())
        {
            return socketInterface->Close();
        }
        return wosler::utilities::Error::SUCCESS;
    }

    virtual void OnProcess() override
    {
        /*
        Thread 1: Read T_dem from UDP
        1. Handshake with Input Device UDP
        2. Read UDP to get T_dem
        3. Save T_dem to memory
        */
        using namespace std::chrono_literals;

        static sockaddr_in sender_config;
        static size_t sender_config_len = sizeof(sender_config);
        memset(&sender_config, 0, sizeof(sender_config));

        try
        {
            if (socketInterface.get())
            {
                // std::cout << "ID waiting for receive...\n";
                error = socketInterface->RecvFrom(
                    raw_data_buffer.data(),
                    buff,
                    reinterpret_cast<sockaddr *>(&sender_config),
                    reinterpret_cast<socklen_t *>(&sender_config_len));
                    
                    if(first) {
                        first = false;
                        buff = packet.GetSize();
                    }
            }

            if (error != wosler::utilities::Error::SUCCESS)
            {
                throw "ID Failed to receive data on socket: " + std::to_string(static_cast<int>(error));
            }
            else
            {
                // std::cout << "Recieved ID: " << raw_data_buffer.data() << "\n";
                // std::cout << "Received ID: ";
                // for(int i = 0; (ssize_t) i < buff; i++) {
                //     std::cout << std::hex << (unsigned int) raw_data_buffer[i] << " ";
                // }
                // std::cout << "\n\n";
                PacketTuple sendTuple = {std::chrono::high_resolution_clock::now(), raw_data_buffer};
                sendQueue.push(sendTuple);
            }
        }
        catch (std::string msg)
        {
            cout_mut.lock();
            std::cout << msg << std::endl;
            cout_mut.unlock();

            throw -1;
        }
    }
};

class SendRunnableID : public wosler::utilities::AsyncRunnable
{
private:
    uint64_t task_counter{0};

    bool handshake{false};

    std::string ip_address;
    std::string target_ip;

    std::unique_ptr<wosler::utilities::UDPSocketInterface> socketInterface;
    sockaddr_in socket_config;
    sockaddr_in target_config;

    std::vector<unsigned char> raw_data_buffer;
    size_t buffer_size_bytes{BUFF_SIZE};

    wosler::utilities::Error error{wosler::utilities::Error::NONE};

    bool first = true;
    int buff = 1;

public:
    SendRunnableID(char **udpParam) : wosler::utilities::AsyncRunnable("Send Communications Runnable")
    {
        ip_address = udpParam[0];
        target_ip = udpParam[1];

        socketInterface = std::make_unique<wosler::utilities::UDPSocketInterface>(
            ip_address,
            PORT_NUMBER,
            BUFF_SIZE);

        raw_data_buffer.resize(buffer_size_bytes);

        socket_config.sin_addr.s_addr = inet_addr(ip_address.data());
        socket_config.sin_family = AF_INET;
        socket_config.sin_port = htons(PORT_NUMBER);
        memset(&(socket_config.sin_zero), 0, 8);

        target_config.sin_addr.s_addr = inet_addr(target_ip.data());
        target_config.sin_family = AF_INET;
        target_config.sin_port = htons(PORT_NUMBER);
        memset(&(target_config.sin_zero), 0, 8);

        cout_mut.lock();
        std::cout << "Send opening socket to " << ip_address << ":" << PORT_NUMBER << "\n";
        cout_mut.unlock();

        if (socketInterface.get())
        {
            error = socketInterface->Open();
        }
        if (error != wosler::utilities::Error::SUCCESS)
        {
            throw std::string("Failed to open socket!");
        }
    };

    ~SendRunnableID()
    {
        if (socketInterface.get())
        {
            socketInterface->Close();
        }
        StopTask();
    };

    virtual void OnProcess() override
    {
        /*
        Thread 0: Send T_curr via UDP
        1. Handshake UDP with Input Device
        2. Get robot's T_curr (current position) from memory
        3. Send T_curr through UDP
        */
        using namespace std::chrono_literals;

        size_t target_config_len = sizeof(socket_config);

        if(!sendQueue.empty() && ((std::chrono::high_resolution_clock::now() - sendQueue.front().recieveTime) > std::chrono::milliseconds(delay_time_id))) {
            try
            {
                PacketTuple pack = sendQueue.front();
                sendQueue.pop();

                // std::cout << "Sending ID: " << pack.data.data() << "\t of size " << buff << "\n";

                if (socketInterface.get())
                {
                    error = socketInterface->SendTo(
                        pack.data.data(),
                        buff,
                        reinterpret_cast<sockaddr *>(&target_config),
                        reinterpret_cast<socklen_t *>(&target_config_len));
                }

                if(first) {
                    first = false;
                    buff = packet.GetSize();
                }

                if (error != wosler::utilities::Error::SUCCESS)
                {
                    throw "ID Failed to send data on socket: " + std::to_string(static_cast<int>(error));
                }
            }
            catch (std::string msg)
            {
                cout_mut.lock();
                std::cout << msg << std::endl;
                cout_mut.unlock();

                throw -1;
            }
        }
    }
};

class InputsRunnable : public wosler::utilities::AsyncRunnable
{
private:
    char command;

public:
    InputsRunnable() : wosler::utilities::AsyncRunnable("Inputs Runnable"){};
    ~InputsRunnable() = default;
    virtual void OnProcess() override
    {
        std::cin >> command;
        if (command == 'q')
        {
            delay_time_rob += 10;
            std::cout << "Delay Time (Rob): " << delay_time_rob << "\n";
        }
        else if (command == 'a')
        {
            delay_time_rob = delay_time_rob >= 10 ? delay_time_rob - 10 : 0;
            std::cout << "Delay Time (Rob): " << delay_time_rob << "\n";
        }
        else if (command == 'w')
        {
            delay_time_id += 10;
            std::cout << "Delay Time (ID): " << delay_time_id << "\n";
        }
        else if (command == 's')
        {
            delay_time_id = delay_time_id >= 10 ? delay_time_id - 10 : 0;
            std::cout << "Delay Time (ID): " << delay_time_id << "\n";
        }
        else if (command == 'z') {
            std::cout << "Enter the desired (Rob) delay time: ";
            uint64_t time;
            std::cin >> time;
            delay_time_rob = time;
            std::cout << "Delay Time (Rob): " << delay_time_rob << "\n";
        }
        else if (command == 'x') {
            std::cout << "Enter the desired (ID) delay time: ";
            uint64_t time;
            std::cin >> time;
            delay_time_id = time;
            std::cout << "Delay Time (ID): " << delay_time_id << "\n";
        }
        else if (command == 'c') {
            std::cout << "Enter the desired total delay time: ";
            uint64_t time;
            std::cin >> time;
            delay_time_id = time / 2;
            delay_time_rob = time / 2;
            std::cout << "Delay Time: " << delay_time_id << "\n";
        }
    }
};

const std::string USAGE = R"raw_usage(
[IP_ADDRESS_1:string] [ROB_IP_ADDRESS:string] [IP_ADDRESS_2:string] [SONOLINK_IP_ADDRESS:string] [RECIEVE_DELAY_MS:int] [SEND_DELAY_MS:int]

To test this app with localhost:
UDPInducedLag 127.0.0.1 127.0.0.2 127.0.0.3 127.0.0.4 100 100
)raw_usage";

int main(int argc, char *argv[]) {
    if(argc != 7) {
        std::cout << argv[0];
        std::cout << USAGE << "\n";
        return 0;
    }

    delay_time_rob = std::stoi(argv[5]);
    delay_time_id = std::stoi(argv[6]);

    SendRunnableRob sendRobTask(&argv[1]);
    RecieveRunnableRob recieveRobTask(&argv[1]);
    SendRunnableID sendIDTask(&argv[3]);
    RecieveRunnableID recieveIDTask(&argv[3]);
    InputsRunnable inputsTask;

    sendRobTask.RunTaskPeriodic(10);
    recieveRobTask.RunTaskPeriodic(10);
    sendIDTask.RunTaskPeriodic(10);
    recieveIDTask.RunTaskPeriodic(10);
    inputsTask.RunTaskPeriodic(100);

    std::this_thread::sleep_for(std::chrono::seconds(21600));

    sendRobTask.StopTask();
    recieveRobTask.StopTask();
    sendIDTask.StopTask();
    recieveIDTask.StopTask();
    inputsTask.StopTask();

    return 0;
};