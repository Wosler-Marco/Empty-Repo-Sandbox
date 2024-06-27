#include "WebSocketClientInterface.hpp"
#include "WebSocketServerInterface.hpp"

int main(int argc, char** argv) {
    if (argc != 4) {
        std::cout << "Not enough arguements\n";
        std::cout << "To use the program: WebSocketTest [IP Address] [Port Number] [CLIENT/SERVER]";
    } else {
        std::string hostName = argv[1];
        std::string hostPort = argv[2];
        std::string type = argv[3];

        if (type.compare("CLIENT") == 0) {
            // Setting up the Websocket client
            net::io_context ioc;
            std::shared_ptr<wosler::utilities::WebSocketClient> client = std::make_shared<wosler::utilities::WebSocketClient>(ioc);
            client->run(hostName, hostPort);
            std::thread t([&ioc]{ioc.run();});

            // While loop to keep getting user input until they quit
            while (true) {
                // Check to see if the client is connected before allowing any operations to happen
                if (client->isConnected()) {
                    std::string input;
                    std::cout << "----------WebSocket Client Test----------\nW: Write\nR: Read\nQ: Quit\n\nInput: ";
                    std::cin >> input;
                    std::cout << "\n";
                    if (input == "w" || input == "W") {
                        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                        std::cout << "Input the string to write: ";
                        std::getline(std::cin, input);
                        std::cout << "\n";
                        client->write(input);
                    } else if (input == "r" || input == "R") {
                        if (client->readQEmpty()) {
                            std::cout << "No messages on the queue...\n\n";
                        } else {
                            std::cout << "Message: " << client->read() << "\n\n";
                        }
                    } else if (input == "q" || input == "Q") {
                        client->close();
                        break;
                    } else {
                        std::cout << "Invalid input\n";
                    }
                } else {
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                }
            }
            // End of while loop

            t.join();
        } else {
            // Setting up the Websocket server
            auto const ipAddress = net::ip::make_address(hostName);
            auto const portNumber = static_cast<unsigned short>(std::atoi(hostPort.c_str()));

            net::io_context ioc;
            std::shared_ptr<wosler::utilities::WebSocketServer> server = std::make_shared<wosler::utilities::WebSocketServer>(ioc, tcp::endpoint{ipAddress, portNumber});
            server->run();
            std::thread t([&ioc]{ioc.run();});

            // While loop to keep getting user input until they quit
            while (true) {
                // Check to see if the server is connected before allowing any operations to begin
                if (server->isConnected()) {
                    std::string input;
                    std::cout << "----------WebSocket Server Test----------\nW: Write\nR: Read\nQ: Quit\n\nInput: ";
                    std::cin >> input;
                    std::cout << "\n";
                    if (input == "w" || input == "W") {
                        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                        std::cout << "Input the string to write: ";
                        std::getline(std::cin, input);
                        std::cout << "\n";
                        server->write(input);
                    } else if (input == "r" || input == "R") {
                        if (server->readQEmpty()) {
                            std::cout << "No messages on the queue...\n\n";
                        } else {
                            std::cout << "Message: " << server->read() << "\n\n";
                        }
                    } else if (input == "q" || input == "Q") {
                        server->close();
                        break;
                    } else {
                        std::cout << "Invalid input\n";
                    }
                } else {
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                }
            }
            // End of while loop

            t.join();
        }
        std::cout << "Program ending...\n\n";
    }
    return 0;
}