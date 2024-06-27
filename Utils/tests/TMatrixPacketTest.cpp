#include <iostream>
#include <string>
#include <chrono>
#include <thread>

#include "TMatrixPacket.hpp"
#include "SerializationInterface.hpp"
#include "SocketInterface.hpp"
#include "AsyncRunnable.hpp"

Eigen::Matrix4f matrix{{0,0,1,100},{0,-1,0,0},{1,0,0,100},{0,0,0,1}};
Eigen::Matrix<float,3,1> force = Eigen::Matrix<float,3,1>::Zero();

class InputsRunnable : public wosler::utilities::AsyncRunnable
{
private:
    char command;

public:
    InputsRunnable() : wosler::utilities::AsyncRunnable("Inputs Runnable"){};
    ~InputsRunnable() = default;
    virtual void OnProcess() override
    {
        std::cout << "Enter a command: ";
        std::cin >> command;
        if (command == 'q') {
            std::cout << "Increasing pos x by 1mm\n\n";
            matrix.coeffRef(0,3) += 1.0;
        }
        else if (command == 'a') {
            std::cout << "Decreasing pos x by 1mm\n\n";
            matrix.coeffRef(0,3) -= 1.0;
        }
        else if (command == 'w') {
            std::cout << "Increasing pos y by 1mm\n\n";
            matrix.coeffRef(1,3) += 1.0;
        }
        else if (command == 's') {
            std::cout << "Decreasing pos y by 1mm\n\n";
            matrix.coeffRef(1,3) -= 1.0;
        }
        else if (command == 'e') {
            std::cout << "Increasing pos z by 1mm\n\n";
            matrix.coeffRef(2,3) += 1.0;
        }
        else if (command == 'd') {
            std::cout << "Decreasing pos z by 1mm\n\n";
            matrix.coeffRef(2,3) -= 1.0;
        }
        else if (command == 'u') {
            std::cout << "Increasing force x by 1mm\n\n";
            force.coeffRef(0,0) += 1.0;
        }
        else if (command == 'j') {
            std::cout << "Decreasing force x by 1mm\n\n";
            force.coeffRef(0,0) -= 1.0;
        }
        else if (command == 'i') {
            std::cout << "Increasing force y by 1mm\n\n";
            force.coeffRef(1,0) += 1.0;
        }
        else if (command == 'k') {
            std::cout << "Decreasing force y by 1mm\n\n";
            force.coeffRef(1,0) -= 1.0;
        }
        else if (command == 'o') {
            std::cout << "Increasing force z by 1mm\n\n";
            force.coeffRef(2,0) += 1.0;
        }
        else if (command == 'l') {
            std::cout << "Decreasing force z by 1mm\n\n";
            force.coeffRef(2,0) -= 1.0;
        }       
    }
};

const std::string USAGE = R"raw_usage(
[IP_ADDRESS:string] [PORT_NUMBER:int] [TARGET_IP_ADDRESS:string] [TARGET_PORT_NUMBER:int] [CLIENT/SERVER]
To test this app with localhost:
udpSocketTest 127.0.0.1 5001 127.0.0.1 5001 SERVER
)raw_usage";

int main(int argc, const char *argv[]) {
    if(argc != 6) {
        std::cout << argv[0];
        std::cout << USAGE << "\n";
        return 0;
    }

    InputsRunnable inputsTask;
    inputsTask.RunTaskPeriodic(100);

    wosler::utilities::TmatrixPacket packet, packet2;
    const std::string ip_address = argv[1];
    const std::string port_number = argv[2];
    const std::string target_ip = argv[3];
    const std::string target_port = argv[4];
    const std::string client_or_server = argv[5];

    sockaddr_in socket_config;
    socket_config.sin_addr.s_addr = inet_addr(ip_address.data());
    socket_config.sin_family = AF_INET;
    socket_config.sin_port = htons(stoi(port_number));
    memset(&(socket_config.sin_zero), 0, 8);

    sockaddr_in target_config;
    target_config.sin_addr.s_addr = inet_addr(target_ip.data());
    target_config.sin_family = AF_INET;
    target_config.sin_port = htons(stoi(target_port));
    size_t target_config_len = sizeof(socket_config);
    memset(&(target_config.sin_zero), 0, 8);

    std::cout << "Opening socket as " << client_or_server << ": " << ip_address << ":" << port_number << "\n";

    wosler::utilities::UDPSocketInterface socketInterface(
        ip_address,
        static_cast<uint16_t>(stoi(port_number)),
        sizeof(wosler::utilities::TmatrixPacket)
    );

    wosler::utilities::Error error = wosler::utilities::Error::NONE;
    
    std::cout << "Opening Socket...\n";
    error = socketInterface.Open();
    if(error != wosler::utilities::Error::SUCCESS) {
        std::cout << "Failed to open socket!\n";
        return -1;
    }

    std::cout << "Binding Socket...\n";
    socketInterface.Bind();
    if(error != wosler::utilities::Error::SUCCESS) {
        std::cout << "Failed to bind socket!\n";
        return -1;
    }
    
    size_t buffer_size_bytes = 256;
    
    std::vector<char> raw_data_buffer;
    raw_data_buffer.resize(buffer_size_bytes);

    while(true) {
        static uint32_t packet_counter = 0;
        static sockaddr_in sender_config;
        static size_t sender_config_len = sizeof(sender_config);
        memset(&sender_config, 0, sizeof(sender_config));

        packet.SetMatrix(matrix);
        packet.SetForces(force);
        packet.SetID(packet_counter);

        if(client_or_server.compare("SERVER") == 0) {
            if (!packet.Serialize(raw_data_buffer.data(), buffer_size_bytes))
            {
                throw std::string("Buffer size too small for packet!");
            }
            
            // std::cout << "Sending Packet " << packet_counter << " with data: " << packet.GetData().Tmatrix.coeff(0, 0) << "\n";
            error = socketInterface.SendTo(
                raw_data_buffer.data(),
                sizeof(packet.GetData()),
                reinterpret_cast<sockaddr*>(&target_config),
                reinterpret_cast<socklen_t*>(&target_config_len)
            );
            if(error != wosler::utilities::Error::SUCCESS) {
                std::cout << "Failed to send data on socket: " << static_cast<int>(error) << "\n";
            } else {
                // std::cout << "Successfully sent packet!\n";
                std::cout << "Sent...\n";
                packet.PrintData();
                std::cout << "\n";
            }
        }

        // std::cout << "Waiting to receive a packet...\n";
        error = socketInterface.RecvFrom(
            raw_data_buffer.data(),
            sizeof(packet.GetData()),
            reinterpret_cast<sockaddr*>(&sender_config),
            reinterpret_cast<socklen_t*>(&sender_config_len)
        );
        if(error != wosler::utilities::Error::SUCCESS) {
            std::cout << "Failed to receive data on socket: " << static_cast<int>(error) << "\n";
        } else {
            packet2.Deserialize(raw_data_buffer.data(), buffer_size_bytes);
            std::cout << "Received...\n";
            packet2.PrintData();
            std::cout << "\n";
            // std::cout << "Received Packet " << packet_counter << ": " << packet2.GetData().Tmatrix.coeff(0, 0);
            // std::cout << " from " << inet_ntoa(sender_config.sin_addr) << ":" << ntohs(sender_config.sin_port) << "\n";
        }
        
        // Eigen::Matrix4f tempData = packet.GetMatrix();
        // tempData(0, 0) = tempData(0, 0) + 1;
        // packet.SetMatrix(tempData);

        if(client_or_server.compare("CLIENT") == 0) {
            if (!packet.Serialize(raw_data_buffer.data(), buffer_size_bytes))
            {
                throw std::string("Buffer size too small for packet!");
            }
            
            std::cout << "Sending Packet " << packet_counter << " with data: " << packet.GetData().Tmatrix.coeff(0, 0) << "\n";
            error = socketInterface.SendTo(
                raw_data_buffer.data(),
                sizeof(packet.GetData()),
                reinterpret_cast<sockaddr*>(&target_config),
                reinterpret_cast<socklen_t*>(&target_config_len)
            );
            if(error != wosler::utilities::Error::SUCCESS) {
                std::cout << "Failed to send data on socket: " << static_cast<int>(error) << "\n";
            } else {
                std::cout << "Successfully sent packet!\n";
            }
        }

        std::cout << std::endl;
        packet_counter++;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    socketInterface.Close();
    inputsTask.StopTask();

    return 0;
};