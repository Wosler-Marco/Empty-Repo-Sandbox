#include <iostream>
#include <boost/beast.hpp>
#include <boost/beast/core.hpp>

#include "CommonUtilities.hpp"
#include "AsyncRunnable.hpp"
#include "JSON/json.hpp"
#include "HTTPWebSocketInterface.hpp"

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;

enum {
    EnterSession = 1,
    StartingWS,
    Running
};

int _step = 0;
std::string _server_addr;
std::string _sessionToken;
// The io_context is required for all I/O
net::io_context _ioc;
std::shared_ptr<wosler::utilities::HTTPWebSocket> _ws;
std::thread _t;

wosler::utilities::Error HTTPSend(std::string addr, http::request<http::string_body>& req, http::response<http::string_body>& res) {
    // Setting up the https stream
    net::io_context ioc;
    
    ssl::context ctx(ssl::context::tlsv12_client);
    // Load CA certificates for server verification
    // ctx.load_verify_file("path/to/ca/cert.pem");
    
    tcp::resolver resolver(ioc);
    
    // Resolve the server address
    auto const results = resolver.resolve(addr, "https");
        
    // Create and connect a socket
    ssl::stream<tcp::socket> stream(ioc, ctx);
    net::connect(stream.next_layer(), results.begin(), results.end());
    stream.handshake(ssl::stream_base::client);

    http::write(stream, req);
    
    // Receive the HTTPS response
    beast::flat_buffer buffer;
    size_t readSize = http::read(stream, buffer, res);
    // std::cout << res << "\n\n";

    beast::error_code ec;
    stream.shutdown(ec);
    stream.next_layer().close();

    ioc.run();

    if(readSize != 0) {
        return wosler::utilities::Error::SUCCESS;
    }
    else {
        return wosler::utilities::Error::FAILED;
    }
}

class InputsRunnable : public wosler::utilities::AsyncRunnable
{
private:
    std::string command;

public:
    InputsRunnable() : wosler::utilities::AsyncRunnable("Inputs Runnable"){};
    ~InputsRunnable() = default;
    virtual void OnProcess() override
    {
        if(_step == EnterSession) {
            nlohmann::json sessionInfo;
            // std::cout << "Patient ID: ";
            // std::getline(std::cin, command);
            // sessionInfo["patientID"] = command;
            sessionInfo["patientID"] = "123";
            
            // std::cout << "Device ID: ";
            // std::getline(std::cin, command);
            // sessionInfo["deviceID"] = command;
            sessionInfo["deviceID"] = "1";

            // std::cout << "Device Token: ";
            // std::getline(std::cin, command);
            // sessionInfo["deviceToken"] = command;
            sessionInfo["deviceToken"] = "skdjfhk3453sdrerkjfhsdkjf";

            // Prepare the HTTP request
            http::request<http::string_body> req{http::verb::post, "/api/sonobot", 11};
            req.set(http::field::host, _server_addr);
            req.set(http::field::user_agent, "Wosler-SonoStation-Test/1.0");
            req.set(http::field::content_type, "application/json");
            req.keep_alive(false);
            req.body() = nlohmann::to_string(sessionInfo);
            req.prepare_payload();

            // Send the HTTP request and wait for response
            http::response<http::string_body> res;
            HTTPSend(_server_addr, req, res);

            // std::cout << res.body() << "\n\n";
            // if successful
            _step = StartingWS;
            nlohmann::json response_body = nlohmann::json::parse(res.body());
            _sessionToken = response_body["token"].get<std::string>();
            // std::cout << "Starting session!\nSession Token: " << _sessionToken << "\n\n";
        }
        else if(_step == StartingWS) {
            std::cout << "Sending WS Upgrade Request...\n";

            // The SSL context is required, and holds certificates
            ssl::context ctx(ssl::context::tlsv12_client);
            _ws = std::make_shared<wosler::utilities::HTTPWebSocket>(_ioc,ctx);
            _ws->run(_server_addr, _sessionToken);
            _t = std::thread([&_ioc]{_ioc.run();});
            _step = Running;
        }
        else if (_step == Running) {
            nlohmann::json messageJSON;
            std::cout << "Enter Key: ";
            std::getline(std::cin, command);
            std::string key = command;
            std::cout << "Enter Value: ";
            std::getline(std::cin, command);
            messageJSON[key] = command;

            // Send the message
            _ws->write(nlohmann::to_string(messageJSON));

            std::cout << "\n";
        }
    }
};

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cout << "Invalid number of arguments\n";
        std::cout << "To use the program: HTTP_WSS_Test [Server Address]";
    } else {
        _server_addr = argv[1];

        InputsRunnable inputTask;
        inputTask.RunTaskPeriodic(100);

        _step = EnterSession;

        // While loop to keep getting websocket responses
        while (_step != Running) { std::this_thread::sleep_for(std::chrono::milliseconds(100)); }
        while (_step == Running) {
            // This buffer will hold the incoming message
            beast::flat_buffer buffer;
            beast::error_code ec;

            try{
                // Read a message into our buffer
                std::string msg = _ws->read();
                if(msg.compare("") != 0) {
                    auto res = nlohmann::json::parse(msg);

                    std::cout << "\n\nReceived:\n" << res << "\n\n";
                }
            }
            catch(std::exception const& e)
            {
                std::cerr << "Error: " << e.what() << std::endl;
                return EXIT_FAILURE;
            }
        }
        
        std::cout << "Program ending...\n\n";
        
        inputTask.StopTask();

        // Close the WebSocket connection when done
        _ws->close();
        _t.join();
    }
    return 0;
}