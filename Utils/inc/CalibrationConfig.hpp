#pragma once 

#include "JSON/json.hpp"

typedef int16_t Step;

namespace wosler {
namespace utilities {
namespace Haply {

    nlohmann::json calibrationConfig = nlohmann::json{
        {"step", {
            {
                {"id", 1},
                {"description", "Place Quill in holder and put the Inverse3 base in a comfortable position"},
                {"graphic", "/assets/input_device/haply/Step1.gif"},
                {"user_options", std::vector<nlohmann::json>{
                    {{"text", "OK"}},
                    {{"text", "Abort"}}
                }}
            },
            {
                {"id", 2},
                {"description", "Place Quill in the Inverse3 and move the arms to an area where movement is possible"},
                {"graphic", "/assets/input_device/haply/Step2.gif"},
                {"user_options", std::vector<nlohmann::json>{
                    {{"text", "OK"}},
                    {{"text", "Abort"}}
                }}
            },
            {
                {"id", 3},
                {"description", "Hang the Quill upside down and ensure it is not swinging"},
                {"graphic", "/assets/input_device/haply/Step3.gif"},
                {"user_options", std::vector<nlohmann::json>{
                    {{"text", "OK"}},
                    {{"text", "Abort"}}
                }}
            },
            {
                {"id", 4},
                {"description", "Move the Quill towards the the patient's head"},
                {"graphic", "/assets/input_device/haply/Step4.gif"},
                {"user_options", std::vector<nlohmann::json>{
                    {{"text", "OK"}},
                    {{"text", "Abort"}}
                }}
            },
            {
                {"id", 5},
                {"description", "Calibration complete!"},
                {"graphic", ""},
                {"user_options", std::vector<nlohmann::json>{
                    {{"text", "OK"}}
                }}
            }
        }}
    };

} // namepsace Haply

namespace test {

    nlohmann::json calibrationConfig = nlohmann::json{
        {"step", {
            {
                {"id", 1},
                {"description", "How I felt starting the GUI"},
                {"graphic", "/assets/input_device/test/smart.gif"},
                {"user_options", std::vector<nlohmann::json>{
                    {{"text", "OK"}},
                    {{"text", "Abort"}}
                }}
            },
            {
                {"id", 2},
                {"description", "Me after days spent working on the Python wrapper workarounds"},
                {"graphic", "/assets/input_device/test/elmo.gif"},
                {"user_options", std::vector<nlohmann::json>{
                    {{"text", "OK"}},
                    {{"text", "Abort"}}
                }}
            },
            {
                {"id", 3},
                {"description", "Me after weeks spent working on the wrapper"},
                {"graphic", "/assets/input_device/test/fine.gif"},
                {"user_options", std::vector<nlohmann::json>{
                    {{"text", "OK"}},
                    {{"text", "Abort"}}
                }}
            },
            {
                {"id", 4},
                {"description", "Me getting to work converting to C++"},
                {"graphic", "/assets/input_device/test/programmer.gif"},
                {"user_options", std::vector<nlohmann::json>{
                    {{"text", "OK"}},
                    {{"text", "Abort"}}
                }}
            },
            {
                {"id", 5},
                {"description", "Now that C++ seems to be working"},
                {"graphic", "/assets/input_device/test/nice.gif"},
                {"user_options", std::vector<nlohmann::json>{
                    {{"text", "OK"}},
                    {{"text", "Abort"}}
                }}
            },
            {
                {"id", 6},
                {"description", "Cat."},
                {"graphic", "/assets/input_device/test/cat.gif"},
                {"user_options", {
                    {{"text", "Finish Calibration"}},
                    {{"text", "Abort"}}
                }}
            }
        }}
    };
  
} // namespace test
} // namespace utilities
} // namespace wosler