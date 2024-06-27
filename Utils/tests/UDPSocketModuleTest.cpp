#include "AsyncRunnable.hpp"
#include "SocketInterface.hpp"
#include "IPCMessage.hpp"
#include "Eigen/Dense"
#include "TMatrixPacket.hpp"

wosler::utilities::EventRequestMessage msg;
wosler::utilities::TmatrixPacket packet;

bool doOnce = false;
bool handshake_start = false;
bool handshake_end = false;
bool packet_send = false;

Eigen::Matrix4f matrix{{0,0,1,100},{0,-1,0,0},{1,0,0,100},{0,0,0,1}};
Eigen::Matrix<float,3,1> force = Eigen::Matrix<float,3,1>::Zero();

uint32_t packet_counter = 0;

class InputsRunnable : public wosler::utilities::AsyncRunnable
{
private:
    char command;

public:
    InputsRunnable() : wosler::utilities::AsyncRunnable("Inputs Runnable"){};
    ~InputsRunnable() = default;
    virtual void OnProcess() override
    {
        ModuleID process;
        FunctionID function;
        EventID event;
        int response;

        std::cin >> command;
        if (command == 'z')
        {
            doOnce = true;
        }
        else if (command == 'x') {
            handshake_start = true;
            doOnce = true;
        }
        else if (command == 'c') {
            handshake_end = true;
            doOnce = true;
        }
        else if (command == 'v') {
            packet_send = true;
            doOnce = true;
        }
        else if (command == 'q') {
            std::cout << "Enter a process ID: ";
            std::cin >> process;
            msg.SetModuleID(process);
        }
        else if (command == 'w') {
            std::cout << "Enter a function ID: ";
            std::cin >> function;
            msg.SetFunctionID(function);
        }
        else if (command == 'e') {
            std::cout << "Enter a event ID: ";
            std::cin >> event;
            msg.SetEventID(event);
        }
        else if (command == 'r') {
            bool enteringData = true;
            char data[msg.GetDataSize()] = "";
            int dataStart = 0;
            while(enteringData) {
                std::cout << "Choose a data type:\nq - Matrix4f\nw - Vector3f\ne - int32_t\nr - int16_t\nt - int8_t\ny - float\nu - bool\nAny other key - Finish Input\n";
                std::cin >> command;
                switch(command) {
                    case 'q': {
                        Eigen::Matrix4f matrix = Eigen::Matrix4f::Zero();
                        if((dataStart + matrix.size()*sizeof(float)) < msg.GetDataSize()) {
                            std::cout << "Enter Matrix4f data:\n";
                            for(int i = 0; i < 16; i++) {
                                float val;
                                std::cout << "(" << (int)(i/4) << "," << (i%4) << ")\t";
                                std::cin >> val;
                                matrix((int)(i/4),i%4) = val;
                            }
                            std::cout << "Total matrix:\n";
                            std::cout << matrix << "\n";
                            std::memcpy(&data[dataStart],matrix.data(),matrix.size()*sizeof(float));
                            dataStart += matrix.size()*sizeof(float);
                        }
                        else {
                            std::cout << "Not enough space for Matrix4f data...\n";
                            std::cout << "Data buffer size: " << msg.GetDataSize() << "\n";
                            std::cout << "Bytes left in buffer: " << msg.GetDataSize()-dataStart << "\n";
                        }
                        break;
                    }
                    case 'w': {
                        Eigen::Matrix<float,3,1> matrix = Eigen::Matrix<float,3,1>::Zero();
                        if((dataStart + matrix.size()*sizeof(float)) < msg.GetDataSize()) {
                            std::cout << "Enter Vector3f data:\n";
                            for(int i = 0; i < 3; i++) {
                                float val;
                                std::cout << "(" << i << ",0)\t";
                                std::cin >> val;
                                matrix(i,0) = val;
                            }
                            std::cout << "Total vector:\n";
                            std::cout << matrix << "\n";
                            std::memcpy(&data[dataStart],matrix.data(),matrix.size()*sizeof(float));
                            dataStart += matrix.size()*sizeof(float);
                        }
                        else {
                            std::cout << "Not enough space for Vector3f data...\n";
                            std::cout << "Data buffer size: " << msg.GetDataSize() << "\n";
                            std::cout << "Bytes left in buffer: " << msg.GetDataSize()-dataStart << "\n";
                        }
                        break;
                    }
                    case 'e': {
                        if((dataStart + sizeof(int32_t)) < msg.GetDataSize()) {
                            std::cout << "Enter int32_t data:\n";
                            int32_t val;
                            std::cin >> val;
                            std::memcpy(&data[dataStart],&val,sizeof(int32_t));
                            dataStart += sizeof(int32_t);
                        }
                        else {
                            std::cout << "Not enough space for int32_t data...\n";
                            std::cout << "Data buffer size: " << msg.GetDataSize() << "\n";
                            std::cout << "Bytes left in buffer: " << msg.GetDataSize()-dataStart << "\n";
                        }
                        break;
                    }
                    case 'r': {
                        if((dataStart + sizeof(int16_t)) < msg.GetDataSize()) {
                            std::cout << "Enter int16_t data:\n";
                            int16_t val;
                            std::cin >> val;
                            std::memcpy(&data[dataStart],&val,sizeof(int16_t));
                            dataStart += sizeof(int16_t);
                        }
                        else {
                            std::cout << "Not enough space for int16_t data...\n";
                            std::cout << "Data buffer size: " << msg.GetDataSize() << "\n";
                            std::cout << "Bytes left in buffer: " << msg.GetDataSize()-dataStart << "\n";
                        }
                        break;
                    }
                    case 't': {
                        if((dataStart + sizeof(int8_t)) < msg.GetDataSize()) {
                            std::cout << "Enter int8_t data:\n";
                            int8_t val;
                            std::cin >> val;
                            std::memcpy(&data[dataStart],&val,sizeof(int8_t));
                            dataStart += sizeof(int8_t);
                        }
                        else {
                            std::cout << "Not enough space for int8_t data...\n";
                            std::cout << "Data buffer size: " << msg.GetDataSize() << "\n";
                            std::cout << "Bytes left in buffer: " << msg.GetDataSize()-dataStart << "\n";
                        }
                        break;
                    }
                    case 'y': {
                        if((dataStart + sizeof(float)) < msg.GetDataSize()) {
                            std::cout << "Enter float data:\n";
                            float val;
                            std::cin >> val;
                            std::memcpy(&data[dataStart],&val,sizeof(float));
                            dataStart += sizeof(float);
                        }
                        else {
                            std::cout << "Not enough space for float data...\n";
                            std::cout << "Data buffer size: " << msg.GetDataSize() << "\n";
                            std::cout << "Bytes left in buffer: " << msg.GetDataSize()-dataStart << "\n";
                        }
                        break;
                    }
                    case 'u': {
                        if((dataStart + sizeof(bool)) < msg.GetDataSize()) {
                            std::cout << "Enter bool:\n";
                            int val;
                            std::cin >> val;
                            bool val_bool = true;
                            if(val == 0) {
                                val_bool = false;
                            }
                            std::memcpy(&data[dataStart],&val_bool,sizeof(bool));
                            dataStart += sizeof(bool);
                        }
                        else {
                            std::cout << "Not enough space for float data...\n";
                            std::cout << "Data buffer size: " << msg.GetDataSize() << "\n";
                            std::cout << "Bytes left in buffer: " << msg.GetDataSize()-dataStart << "\n";
                        }
                        break;
                    }
                    default: {
                        std::cout << "Stopping data input...\n";
                        enteringData = false;
                        break;
                    }
                }
            }
            msg.SetDataBuffer(data, sizeof(data));
        }
        else if (command == 't') {
            std::cout << "Is a response required: ";
            std::cin >> response;
            if(response) { msg.SetResponseFlag(true); }
            else { msg.SetResponseFlag(false); }
        }
    }
};

class SendRunnable : public wosler::utilities::AsyncRunnable
{
private:
    std::string ip_address;
    std::string target_ip;
    int target_port;

    std::unique_ptr<wosler::utilities::UDPSocketInterface> socketInterface;
    sockaddr_in socket_config;
    sockaddr_in target_config;

    size_t buff_size;

    wosler::utilities::Error error{wosler::utilities::Error::NONE};

public:
    SendRunnable(char **udpParam) : wosler::utilities::AsyncRunnable("Send Communications Runnable")
    {
        wosler::utilities::EventRequestMessage msg;
        buff_size = msg.GetMessageSize()*10;
        
        ip_address = udpParam[0];
        target_port = std::stoi(udpParam[1]);
        
        socketInterface = std::make_unique<wosler::utilities::UDPSocketInterface>(
            ip_address,
            4500,
            buff_size);
        
        socket_config.sin_addr.s_addr = inet_addr(ip_address.data());
        socket_config.sin_family = AF_INET;
        socket_config.sin_port = htons(4500);
        memset(&(socket_config.sin_zero), 0, 8);
        
        target_config.sin_addr.s_addr = inet_addr(ip_address.data());
        target_config.sin_family = AF_INET;
        target_config.sin_port = htons(target_port);
        memset(&(target_config.sin_zero), 0, 8);
        
        std::cout << "Send opening socket to " << ip_address << ":" << target_port << "\n";

        if (socketInterface.get())
        {
            error = socketInterface->Open();
        }
        if (error != wosler::utilities::Error::SUCCESS)
        {
            #ifdef _WIN32
            const char* errorNo = std::to_string(WSAGetLastError()).c_str();
            #else
            const char* errorNo = strerror(errno);
            #endif
            throw std::string("Failed to open socket: " + std::to_string(static_cast<int>(error)) + ": " + errorNo);
        }
    };

    ~SendRunnable()
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

        try
        {
            if(doOnce) {
                doOnce = false;

                std::vector<char> sendPacket;
                sendPacket.resize(buff_size);
                
                if (handshake_start) {
                    sendPacket[0] = 'H';
                }
                else if (handshake_end) {
                    sendPacket[1] = 'S';
                }
                else if (packet_send) {
                    packet.SetMatrix(matrix);
                    packet.SetForces(force);
                    packet.SetID(packet_counter++);

                    if (!packet.Serialize(sendPacket.data(), buff_size))
                    {
                        throw std::string("Buffer size too small for packet!");
                    }
                }
                else {
                    auto current_time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
                    auto current_time_msec = current_time.count();
                    msg.SetTime(current_time_msec);

                    if (!msg.Serialize(sendPacket.data(), buff_size))
                    {
                        throw std::string("Buffer size too small for packet!");
                    }
                }

                if (socketInterface.get())
                {
                    size_t size = 1;
                    if(handshake_start || handshake_end) {
                        handshake_start = false;
                        handshake_end = false;
                    }
                    else if (packet_send) {
                        packet_send = false;
                        size = packet.GetSize();
                    }
                    else {
                        size = msg.GetMessageSize();
                    }

                    std::cout << "Sending packet: ";
                    // msg.printMessage();
                    // std::cout << msg.GetMessageSize() << "\n";
                    std::cout << size << "\n";
                    for(int i = 0; (size_t) i < size; i++) {
                        std::cout << std::hex << (unsigned int) sendPacket[i] << " ";
                    }
                    std::cout << std::dec << " to " << inet_ntoa(target_config.sin_addr) << ":" << ntohs(target_config.sin_port) << "\n";

                    error = socketInterface->SendTo(
                        sendPacket.data(),
                        size,
                        reinterpret_cast<sockaddr *>(&target_config),
                        reinterpret_cast<socklen_t *>(&target_config_len));
                }

                if (error != wosler::utilities::Error::SUCCESS)
                {
                    throw "Failed to send data on socket: " + std::to_string(static_cast<int>(error));
                }
            }
        }
        catch (std::string msg)
        {
            std::cout << msg << std::endl;
            throw -1;
        }
    }
};

const std::string USAGE = R"raw_usage(
[IP_ADDRESS:string] [TARGET_PORT_NUMBER:string]

Default socket port number is 4500

To test this app:
UDPSocketModuleTest 127.0.0.1 5002
)raw_usage";

int main(int argc, char **argv)
{
    if (argc != 3)
    {
        std::cout << argv[0];
        std::cout << USAGE << "\n";
        return 0;
    }
    
    msg.SetModuleID(99);
    msg.SetFunctionID(3);
    msg.SetEventID(8);
    msg.SetResponseFlag(true);
    Eigen::Matrix4f matrix{{1, 2, 3, 4}, {5, 6, 7, 8}, {9, 10, 11, 12}, {13, 14, 15, 16}};
    std::memcpy(msg.GetDataBuffer(),matrix.data(),matrix.size()*sizeof(float));
    try {
        InputsRunnable inputsTask;
        SendRunnable sendTask(&argv[1]);
        
        inputsTask.RunTaskPeriodic(100);
        sendTask.RunTaskPeriodic(100);
    
        std::this_thread::sleep_for(std::chrono::seconds(21600));

        inputsTask.StopTask();
        sendTask.StopTask();
    }
    catch(std::string err) {
        std::cout << err << "\n";
    }
    return 0;
}