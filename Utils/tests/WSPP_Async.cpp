#include "WSSInterface.hpp"
#include "HTTPSWrite.hpp"

#include "CommonUtilities.hpp"
#include "AsyncRunnable.hpp"
#include "JSON/json.hpp"


enum {
    Login = 1,
    EnterSession,
    StartingWS,
    Running
};

int _step = 0;
int _id = -1;
std::string _server_addr;
std::string _userToken;
std::string _sessionToken;
wosler::utilities::WSSInterface endpoint;

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
            HTTPSWrite(_server_addr, req, res);

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
            HTTPSWrite(_server_addr, req, res);

            _step = StartingWS;
            nlohmann::json response_body = nlohmann::json::parse(res.body());
            _sessionToken = response_body["token"].get<std::string>();

        }
        else if(_step == StartingWS) {
            std::cout << "Sending WS Upgrade Request...\n";
            endpoint.setHeader("token",_sessionToken);
            _id = endpoint.connect(_server_addr);
            if (_id != -1) {
                std::cout << "> Created connection with id " << _id << std::endl;
            } else {
                std::cout << "> Connection Failed " << _id << std::endl;
                return;
            }
            while(endpoint.get_metadata(_id)->get_status() != wosler::utilities::ConnectionStatus::Open) { std::this_thread::sleep_for(std::chrono::milliseconds(100)); }
            _step = Running;
        }
        else if (_step == Running) {
            if (endpoint.get_metadata(_id)->get_status() == wosler::utilities::ConnectionStatus::Open) {
                nlohmann::json messageJSON;
                std::cout << "Enter Key: ";
                std::getline(std::cin, command);
                std::string key = command;
                std::cout << "Enter Value: ";
                std::getline(std::cin, command);
                messageJSON[key] = command;

                std::cout << "\n";

                // Send the message
                try{
                    std::string jsonString = nlohmann::to_string(messageJSON);
                    endpoint.send(_id,jsonString);

                }
                catch(websocketpp::exception const & e)
                {
                    std::cerr << "Error: " << e.what() << std::endl;
                    return;
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(1));
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
        inputTask.RunTaskPeriodic(10);

        _step = Login;
        // int id = _id;
        // std::cout << id << "\n\n";

        // While loop to keep getting websocket responses
        while (_step != Running) { std::this_thread::sleep_for(std::chrono::milliseconds(100)); }
        while (_step == Running) {
            // This buffer will hold the incoming message
            boost::beast::flat_buffer buffer;

            try{

                if(!endpoint.readQEmpty(_id)) {
                    // int id = _id;
                    // std::cout << id << "\n\n";
                    std::string msg = endpoint.read(_id);
                  
                    nlohmann::json res;
                    try {
                        // res = nlohmann::json::from_cbor(boost::beast::buffers_to_string(resBuff));
                        res = nlohmann::json::from_cbor(msg);
                    }
                    catch(std::exception const& e) {
                        // res = nlohmann::json::parse(boost::beast::buffers_to_string(resBuff));
                        try {
                            res = nlohmann::json::parse(msg);
                        }
                        catch(std::exception const& e) {
                            // std::cout << "\n\nFailed to parse or cbor: " << msg << "\n\n";
                            continue;
                        }
                    }
                    if(res.contains("key")) {
                        std::cout << "\n\nReceived:" << res["key"] << "\n\n";
                    }
                    else {
                        std::cout << "\n\nReceived: " << res << "\n\n";
                    }
                }
            }
            catch(std::exception const& e)
            {
                std::cout << "\n" << e.what() << "\n";
                std::cerr << "Error: " << e.what() << std::endl;
                return EXIT_FAILURE;
            }
        }
        
        std::cout << "Program ending...\n\n";
        
        inputTask.StopTask();

        // Close the WebSocket connection when done
        int close_code = websocketpp::close::status::normal;
        std::string reason = "Ended Session";
        endpoint.close(_id, static_cast<websocketpp::close::status::value>(close_code), reason);
        
    }
    return 0;
}