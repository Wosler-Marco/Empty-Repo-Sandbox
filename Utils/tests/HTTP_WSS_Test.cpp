#include <iostream>
#include <boost/beast.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/ssl/ssl_stream.hpp>
#include <boost/beast/core.hpp>
#include <boost/asio/ssl.hpp>
// #include <boost/asio/dispatch.hpp>
// #include <boost/asio/strand.hpp>
#include <random>

#include "CommonUtilities.hpp"
#include "AsyncRunnable.hpp"
#include "JSON/json.hpp"

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;  
namespace ssl = boost::asio::ssl;          // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp; 

enum {
    Login = 1,
    EnterSession,
    StartingWS,
    Running
};

int _step = 0;
std::string _server_addr;
std::string _userToken;
std::string _sessionToken;
// The io_context is required for all I/O
net::io_context _ioc;
std::unique_ptr<websocket::stream<beast::ssl_stream<tcp::socket>>> _ws;

// Function to generate a random base64-encoded string for websockets
std::string generateRandomBase64Key() {
    static const std::string base64_chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(0, 63);

    std::string key;
    key.reserve(22);

    for (int i = 0; i < 22; ++i) {
        key.push_back(base64_chars[dist(gen)]);
    }

    key = key + "==";

    return key;
}

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

wosler::utilities::Error CreateWS(std::string addr, net::io_context& ioc, std::unique_ptr<websocket::stream<beast::ssl_stream<tcp::socket>>>& ws) {
    // The SSL context is required, and holds certificates
    ssl::context ctx(ssl::context::tlsv12_client);
    
    // These objects perform our I/O
    tcp::resolver resolver(ioc);
    ws = std::make_unique<websocket::stream<beast::ssl_stream<tcp::socket>>>(ioc,ctx);
    
    // Resolve the server address
    auto const results = resolver.resolve(addr, "https");
    
    // Make HTTPS connection
    // if(ws.get())
    net::connect(get_lowest_layer(*ws), results);

    // Perform SSL Handshake
    ws->next_layer().handshake(ssl::stream_base::client);

    // Add a header for the session token
    ws->set_option(websocket::stream_base::decorator(
        [](websocket::request_type& req)
        {
            req.set("token", _sessionToken);
        }));
    
    // Perform websocket handshake
    ws->handshake(addr, "/");

    return wosler::utilities::Error::SUCCESS;
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
        if(_step == Login) {
            nlohmann::json loginInfo;
            // std::cout << "Email: ";
            // std::getline(std::cin, command);
            // loginInfo["email"] = command;
            loginInfo["email"] = "admin@wosler.ca";

            // std::cout << "Password: ";
            // std::getline(std::cin, command);
            // loginInfo["password"] = command;
            loginInfo["password"] = "Wosler@1";

            // Prepare the HTTP request
            http::request<http::string_body> req{http::verb::post, "/auth/api/login", 11};
            req.set(http::field::host, _server_addr);
            req.set(http::field::user_agent, "Wosler-SonoStation-Test/1.0");
            req.set(http::field::content_type, "application/json");
            req.keep_alive(false);
            req.body() = nlohmann::to_string(loginInfo);
            req.prepare_payload();

            // Send the HTTP request and wait for response
            http::response<http::string_body> res;
            HTTPSend(_server_addr, req, res);

            // std::cout << res.body() << "\n\n";
            
            nlohmann::json response_body = nlohmann::json::parse(res.body());
            if(response_body.contains("error")) {
                std::cout << response_body["error"].get<std::string>() << "\n\n";
            }
            else {
                if(response_body.contains("token")) {
                    _step = EnterSession;
                    _userToken = response_body["token"].get<std::string>();
                    // std::cout << "Successful Login!\nUser Token: " << _userToken << "\n\n";
                }
            }
        }
        else if(_step == EnterSession) {
            nlohmann::json patientInfo;
            // std::cout << "Patient ID: ";
            // std::getline(std::cin, command);
            // patientInfo["patientID"] = command;
            patientInfo["patientID"] = "123";

            // Prepare the HTTP request
            http::request<http::string_body> req{http::verb::post, "/api/sonostation", 11};
            req.set(http::field::host, _server_addr);
            req.set(http::field::user_agent, "Wosler-SonoStation-Test/1.0");
            req.set(http::field::content_type, "application/json");
            req.keep_alive(false);
            req.set("Cookie","token="+_userToken);
            req.body() = nlohmann::to_string(patientInfo);
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

            wosler::utilities::Error err = CreateWS(_server_addr,_ioc,_ws);
            
            if(err == wosler::utilities::Error::SUCCESS) {
                std::cout << "Successfully created web socket!\n\n";
                _step = Running;
            }
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
            _ws->write(net::buffer(nlohmann::to_string(messageJSON)));

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

        _step = Login;

        // While loop to keep getting websocket responses
        while (_step != Running) { std::this_thread::sleep_for(std::chrono::milliseconds(100)); }
        while (_step == Running) {
            // This buffer will hold the incoming message
            beast::flat_buffer buffer;
            beast::error_code ec;

            try{
                // Read a message into our buffer
                size_t msgSize = _ws->read(buffer,ec);
                // std::cout << "\n\nMessage Size: " << msgSize << "\n";
                if (ec) {
                    std::cerr << "Error code: " << ec.value() << ", Message: " << ec.message() << std::endl;
                    if(ec.value() == 1) {
                        break;
                    }
                    return EXIT_FAILURE;
                }
                if(msgSize != 0) {
                    auto resBuff = buffer.data();
                    auto res = nlohmann::json::parse(beast::buffers_to_string(resBuff));

                    std::cout << "\n\nReceived:\n" << res << "\n\n";
                }
            }
            catch(std::exception const& e)
            {
                std::cout << "\n" << ec << "\n";
                std::cerr << "Error: " << e.what() << std::endl;
                return EXIT_FAILURE;
            }
        }
        
        std::cout << "Program ending...\n\n";
        
        inputTask.StopTask();

        // Close the WebSocket connection when done
        _ws->close(websocket::close_code::normal);
    }
    return 0;
}