#include <iostream>
#include <vector>
#include <chrono>
#include <sstream>
#include "JSON/json.hpp"

namespace wosler {
namespace utilities {

// Abstract class to create a base message object
class message {
    protected:
        std::uint32_t messageID;
        std::uint32_t commandID;
        std::time_t UTCTime;
        std::uint32_t errorCode;

        // Method to convert uint32 to hex for encoding purposes
        static std::string convertToHex(std::uint32_t val) {
            std::array<std::uint8_t, 4> arr;
            arr[0] = val >> 24;
            arr[1] = val >> 16;
            arr[2] = val >> 8;
            arr[3] = val;
            return std::string(std::begin(arr), std::end(arr));
        }

        // Method to convert uint64 to hex for encoding purposes
        static std::string convertToHex(std::uint64_t val) {
            std::array<std::uint8_t, 8> arr;
            arr[0] = val >> 56;
            arr[1] = val >> 48;
            arr[2] = val >> 40;
            arr[3] = val >> 32;
            arr[4] = val >> 24;
            arr[5] = val >> 16;
            arr[6] = val >> 8;
            arr[7] = val;
            return std::string(std::begin(arr), std::end(arr));
        }

        // Method to convert time_t to hex for encoding purposes
        static std::string convertToHex(std::time_t val) {
            std::stringstream stream;
            stream << std::hex << val;
            return std::string(stream.str());
        }

        // Method to convert an array of 16 floats to hex for encoding purposes
        static std::string convertToHex(std::array<float, 16> val) {
            std::string str;
            union { float fval ; std::uint32_t ival ; };

            for (int i = 0; i < 16; i++) {
                fval = val[i];
                std::ostringstream stm;
                stm << std::hex << std::uppercase << ival;
                std::string val = stm.str();
                val.insert(val.begin(), 8 - val.size(), '0');
                str.append(val);
            }

            return str;
        }

        // Method to convert a 4x4 matrix of floats to hex for encoding purposes
        static std::string convertToHex(Eigen::Matrix4f matrix) {
            std::string str;
            union { float fval ; std::uint32_t ival ; };

            for (int j = 0; j < 4; j++) {
                for (int k = 0; k < 4; k++) {
                    fval = matrix.coeffRef(j, k);
                    std::ostringstream stm;
                    stm << std::hex << std::uppercase << ival;
                    std::string val = stm.str();
                    val.insert(val.begin(), 8 - val.size(), '0');
                    str.append(val);
                }
            }

            return str;
        }

        // Method to convert a 3x1 vector of floats to hex for encoding purposes
        static std::string convertToHex(Eigen::Vector3f vector) {
            std::string str;
            union { float fval ; std::uint32_t ival ; };

            for (int j = 0; j < 3; j++) {
                fval = vector.coeffRef(j);
                std::ostringstream stm;
                stm << std::hex << std::uppercase << ival;
                std::string val = stm.str();
                val.insert(val.begin(), 8 - val.size(), '0');
                str.append(val);
            }

            return str;
        }

        // Method to convert hex to uint32 for decoding purposes
        static std::uint32_t convertFromHex(std::string hex) {
            const std::uint8_t *hexPtr = reinterpret_cast<const std::uint8_t*>(hex.c_str());
            return (std::uint32_t)hexPtr[0] << 24 | (std::uint32_t)hexPtr[1] << 16 | (std::uint32_t)hexPtr[2] << 8 | (std::uint32_t)hexPtr[3];
        }

        // Method to convert hex to time_t for decoding purposes
        static std::uint64_t convertTimeFromHex(std::string hex) {
            std::uint64_t t;
            std::stringstream stream;
            std::string timeStr = hex;
            stream << std::hex << timeStr;
            stream >> t;
            return t;
        }

    public:
        // Pure virtual method to print out the message data
        virtual void printMessage() = 0;

        // Pure virtual method to create a string from the JSON data
        virtual std::string createJSONString() = 0;
        
        // Method to get the Message ID data field
        std::uint32_t getMessageID() {
            return messageID;
        }

        // Method to get the Command ID data field
        std::uint32_t getCommandID() {
            return commandID;
        }

        // Method to get the UTC Time data field
        std::time_t getUTCTime() {
            return UTCTime;
        }

        // Method to get the Error Code data field
        std::uint32_t getErrorCode() {
            return errorCode;
        }
};

// Class to create an Authentication Message object
class authenticationMessage: public message {
    private:
        std::string clinicID;
        std::string roboticArm;
        std::string roomID;

    public:
        // Default constructor to allow the from_json method to work
        authenticationMessage() = default;

        // Constructor for the class which initializes all the data fields using decoded values
        authenticationMessage(std::uint32_t message, std::string clinic, std::string robot, std::string room, std::uint32_t error) {
            messageID = message;
            commandID = 1093677105;
            UTCTime = std::time(0);
            clinicID = clinic;
            roboticArm = robot;
            roomID = room;
            errorCode = error;
        }

        // Constructor for the class which initializes all the data fields using encoded values
        authenticationMessage(std::string message, std::string clinic, std::string robot, std::string room, std::string error) {
            messageID = convertFromHex(message);
            commandID = 1093677105;
            UTCTime = std::time(0);
            clinicID = clinic;
            roboticArm = robot;
            roomID = room;
            errorCode = convertFromHex(error);
        }

        // Method to convert a JSON string to an Authentication Message object
        friend void from_json(const nlohmann::json& nlohmann_json_j, authenticationMessage& nlohmann_json_t) {
            nlohmann_json_t.messageID = convertFromHex(nlohmann_json_j.at("messageID"));
            nlohmann_json_t.commandID = convertFromHex(nlohmann_json_j.at("commandID"));
            nlohmann_json_t.UTCTime = convertTimeFromHex(nlohmann_json_j.at("UTCTime"));
            nlohmann_json_t.clinicID = nlohmann_json_j.at("clinic");
            nlohmann_json_t.roboticArm = nlohmann_json_j.at("roboticArm");
            nlohmann_json_t.roomID = nlohmann_json_j.at("room");
            nlohmann_json_t.errorCode = convertFromHex(nlohmann_json_j.at("errorCode"));
        }

        // Method to print out the data in the object
        void printMessage() {
            std::cout << "Message ID: " << messageID << "\n" << "Command ID: " << commandID << "\n" << "UTC Time: " << asctime(gmtime(&UTCTime)) << "Clinic ID: " << clinicID << "\n" << "Robotic Arm: " << roboticArm << "\n"  << "Room ID: " << roomID << "\n"<< "Error Code: " << errorCode << "\n\n";
        }

        // Method to create a JSON string from the Authentication Message object
        std::string createJSONString() {
            std::ostringstream ss;
            ss << "{\"messageID\":\"" << convertToHex(messageID) << "\",\"commandID\":\"A001\",\"UTCTime\":\"" << convertToHex(UTCTime) << "\",\"clinic\":\"" << clinicID << "\",\"roboticArm\":\"" << roboticArm << "\",\"room\":\"" << roomID << "\",\"errorCode\":\"" << convertToHex(errorCode) << "\"}";
            return ss.str();
        }

        // Method to get the Clinic ID data field
        std::string getClinicID() {
            return clinicID;
        }

        // Method to get the Robotic Arm data field
        std::string getRoboticArm() {
            return roboticArm;
        }

        // Method to get the Room ID data field
        std::string getRoomID() {
            return roomID;
        }
};

// Class to create a Control Message object
class controlMessage: public message {
    private:
        Eigen::Matrix4f TMatrix;
        Eigen::Vector3f Force;
    
    public:
        // Default constructor to allow the from_json method to work
        controlMessage() = default;

        // Constructor for the class which initializes all the data fields using decoded values
        controlMessage(std::uint32_t message, Eigen::Matrix4f matrix, Eigen::Vector3f force, std::uint32_t error) {
            messageID = message;
            commandID = 1093677106;
            UTCTime = std::time(0);
            TMatrix = matrix;
            Force = force;
            errorCode = error;
        }

        // Constructor for the class which initializes all the data fields using encoded values
        controlMessage(std::string message, Eigen::Matrix4f matrix, Eigen::Vector3f force, std::string error) {
            messageID = convertFromHex(message);
            commandID = 1093677106;
            UTCTime = std::time(0);
            TMatrix = matrix;
            Force = force;
            errorCode = convertFromHex(error);
        }

        // Method to convert a JSON string to a Control Message object
        friend void from_json(const nlohmann::json& nlohmann_json_j, controlMessage& nlohmann_json_t) {
            nlohmann_json_t.messageID = convertFromHex(nlohmann_json_j.at("messageID"));
            nlohmann_json_t.commandID = convertFromHex(nlohmann_json_j.at("commandID"));
            nlohmann_json_t.UTCTime = convertTimeFromHex(nlohmann_json_j.at("UTCTime"));

            // Converting the T-Matrix from hex to an array of 16 floats
            std::string matrixStr = nlohmann_json_j.at("matrix");
            Eigen::Matrix4f matrix;
            union { float fval ; std::uint32_t ival ; };

            int i = 0;
            for (int j = 0; j < 4; j++) {
                for (int k = 0; k < 4; k++) {
                    std::stringstream stream;
                    std::string val(&matrixStr[i * 8], &matrixStr[(i * 8) + 8]);
                    stream << val;
                    stream >> std::hex >> ival;
                        matrix.coeffRef(j, k) = fval;
                        i++;
                }
            }

            nlohmann_json_t.TMatrix = matrix;

            std::string forceStr = nlohmann_json_j.at("force");
            Eigen::Vector3f force;
            for (int i = 0; i < 3; i++) {
                std::stringstream stream;
                std::string val(&forceStr[i * 8], &forceStr[(i * 8) + 8]);
                stream << val;
                stream >> std::hex >> ival;
                force.coeffRef(i) = fval;
            }

            nlohmann_json_t.Force = force;

            nlohmann_json_t.errorCode = convertFromHex(nlohmann_json_j.at("errorCode"));
        }

        // Method to print out the data in the object
        void printMessage() {
            std::cout << "Message ID: " << messageID << "\n";
            std::cout << "Command ID: " << commandID << "\n";
            std::cout << "UTC Time: " << asctime(gmtime(&UTCTime));
            std::cout << "Error Code: " << errorCode << "\n";
            std::cout << "T-Matrix:\n" << TMatrix << "\n";
            std::cout << "Force:\n" << Force << "\n\n";
        }

        // Method to create a JSON string from the Control Message object
        std::string createJSONString() {
            std::ostringstream ss;
            ss << "{\"messageID\":\"" << convertToHex(messageID) << "\",\"commandID\":\"A002\",\"UTCTime\":\"" << convertToHex(UTCTime) << "\",\"matrix\":\"" << convertToHex(TMatrix) << "\",\"force\":\"" << convertToHex(Force) << "\",\"errorCode\":\"" << convertToHex(errorCode) << "\"}";
            return ss.str();
        }
        
        // Method to get the T-Matrix data field
        Eigen::Matrix4f getTMatrix() {
            return TMatrix;
        }

        Eigen::Vector3f getForce() {
            return Force;
        }
};

// Class to create a Tool Change Message object
class toolMessage: public message {
    private:
        std::string toolID;

    public:
        // Default constructor to allow the from_json method to work
        toolMessage() = default;

        // Constructor for the class which initializes all the data fields using decoded values
        toolMessage(std::uint32_t message, std::string tool, std::uint32_t error) {
            messageID = message;
            commandID = 1093677107;
            UTCTime = std::time(0);
            toolID = tool;
            errorCode = error;
        }

        // Constructor for the class which initializes all the data fields using encoded values
        toolMessage(std::string message, std::string tool, std::string error) {
            messageID = convertFromHex(message);
            commandID = 1093677107;
            UTCTime = std::time(0);
            toolID = tool;
            errorCode = convertFromHex(error);
        }

        // Method to convert a JSON string to a Tool Change Message object
        friend void from_json(const nlohmann::json& nlohmann_json_j, toolMessage& nlohmann_json_t) {
            nlohmann_json_t.messageID = convertFromHex(nlohmann_json_j.at("messageID"));
            nlohmann_json_t.commandID = convertFromHex(nlohmann_json_j.at("commandID"));
            nlohmann_json_t.UTCTime = convertTimeFromHex(nlohmann_json_j.at("UTCTime"));
            nlohmann_json_t.toolID = nlohmann_json_j.at("tool");
            nlohmann_json_t.errorCode = convertFromHex(nlohmann_json_j.at("errorCode"));
        }

        // Method to print out the data in the object
        void printMessage() {
            std::cout << "Message ID: " << messageID << "\n" << "Command ID: " << commandID << "\n" << "UTC Time: " << asctime(gmtime(&UTCTime)) << "Tool ID: " << toolID << "\n" << "Error Code: " << errorCode << "\n\n";
        }

        // Method to create a JSON string from the Tool Change Message object
        std::string createJSONString() {
            std::ostringstream ss;
            ss << "{\"messageID\":\"" << convertToHex(messageID) << "\",\"commandID\":\"A003\",\"UTCTime\":\"" << convertToHex(UTCTime) << "\",\"tool\":\"" << toolID << "\",\"errorCode\":\"" << convertToHex(errorCode) << "\"}";
            return ss.str();
        }

        // Method to get the Tool ID data field
        std::string getToolID() {
            return toolID;
        }
};

// Class to create a Gel Dispense Message object
class gelDispenseMessage: public message {
    private:
        std::string amount;

    public:
        // Default constructor to allow the from_json method to work
        gelDispenseMessage() = default;

        // Constructor for the class which initializes all the data fields using decoded values
        gelDispenseMessage(std::uint32_t message, std::string gel, std::uint32_t error) {
            messageID = message;
            commandID = 1093677108;
            UTCTime = std::time(0);
            amount = gel;
            errorCode = error;
        }

        // Constructor for the class which initializes all the data fields using encoded values
        gelDispenseMessage(std::string message, std::string gel, std::string error) {
            messageID = convertFromHex(message);
            commandID = 1093677108;
            UTCTime = std::time(0);
            amount = gel;
            errorCode = convertFromHex(error);
        }

        // Method to convert a JSON string to a Gel Dispense Message object
        friend void from_json(const nlohmann::json& nlohmann_json_j, gelDispenseMessage& nlohmann_json_t) {
            nlohmann_json_t.messageID = convertFromHex(nlohmann_json_j.at("messageID"));
            nlohmann_json_t.commandID = convertFromHex(nlohmann_json_j.at("commandID"));
            nlohmann_json_t.UTCTime = convertTimeFromHex(nlohmann_json_j.at("UTCTime"));
            nlohmann_json_t.amount = nlohmann_json_j.at("amount");
            nlohmann_json_t.errorCode = convertFromHex(nlohmann_json_j.at("errorCode"));
        }

        // Method to print out the data in the object
        void printMessage() {
            std::cout << "Message ID: " << messageID << "\n" << "Command ID: " << commandID << "\n" << "UTC Time: " << asctime(gmtime(&UTCTime)) << "Amount of gel: " << amount << "\n" << "Error Code: " << errorCode << "\n\n";
        }

        // Method to create a JSON string from the Gel Dispense Message object
        std::string createJSONString() {
            std::ostringstream ss;
            ss << "{\"messageID\":\"" << convertToHex(messageID) << "\",\"commandID\":\"A004\",\"UTCTime\":\"" << convertToHex(UTCTime) << "\",\"amount\":\"" << amount << "\",\"errorCode\":\"" << convertToHex(errorCode) << "\"}";
            return ss.str();
        }

        // Method to get the Gel Amount data field
        std::string getAmount() {
            return amount;
        }
};

// Class to create a Homing Message object
class homingMessage: public message {
    private:
        std::array<float, 16> TMatrix;
        std::string toolID;
    
    public:
        // Default constructor to allow the from_json method to work
        homingMessage() = default;

        // Constructor for the class which initializes all the data fields using decoded values
        homingMessage(std::uint32_t message, std::array<float, 16> matrix, std::string tool, std::uint32_t error) {
            messageID = message;
            commandID = 1093677109;
            UTCTime = std::time(0);
            TMatrix = matrix;
            toolID = tool;
            errorCode = error;
        }

        // Constructor for the class which initializes all the data fields using encoded values
        homingMessage(std::string message, std::array<float, 16> matrix, std::string tool, std::string error) {
            messageID = convertFromHex(message);
            commandID = 1093677109;
            UTCTime = std::time(0);
            TMatrix = matrix;
            toolID = tool;
            errorCode = convertFromHex(error);
        }

        // Method to convert a JSON string to a Homing Message object
        friend void from_json(const nlohmann::json& nlohmann_json_j, homingMessage& nlohmann_json_t) {
            nlohmann_json_t.messageID = convertFromHex(nlohmann_json_j.at("messageID"));
            nlohmann_json_t.commandID = convertFromHex(nlohmann_json_j.at("commandID"));
            nlohmann_json_t.UTCTime = convertTimeFromHex(nlohmann_json_j.at("UTCTime"));

            // Converting the T-Matrix from hex to an array of 16 floats
            std::string matrixStr = nlohmann_json_j.at("matrix");
            std::array<float, 16> matrixArr;
            union { float fval ; std::uint32_t ival ; };

            for (int i = 0; i < 16; i++) {
                std::stringstream stream;
                std::string val(&matrixStr[i * 8], &matrixStr[(i * 8) + 8]);
                stream << val;
                stream >> std::hex >> ival;
                matrixArr[i] = fval;
            }
            nlohmann_json_t.TMatrix = matrixArr;

            nlohmann_json_t.toolID = nlohmann_json_j.at("tool");
            nlohmann_json_t.errorCode = convertFromHex(nlohmann_json_j.at("errorCode"));
        }

        // Method to print out the data in the object
        void printMessage() {
            std::cout << "Message ID: " << messageID << "\n" << "Command ID: " << commandID << "\n" << "UTC Time: " << asctime(gmtime(&UTCTime)) << "Tool ID: " << toolID << "\n" << "Error Code: " << errorCode << "\n" << "T-Matrix: ";
            
            for (auto i: TMatrix) {
                std::cout << i << ", ";
            }

            std::cout << "\n" << std::endl;
        }

        // Method to create a JSON string from the Homing Message object
        std::string createJSONString() {
            std::ostringstream ss;
            ss << "{\"messageID\":\"" << convertToHex(messageID) << "\",\"commandID\":\"A005\",\"UTCTime\":\"" << convertToHex(UTCTime) << "\",\"matrix\":\"" << convertToHex(TMatrix) << "\",\"tool\":\"" << toolID << "\",\"errorCode\":\"" << convertToHex(errorCode) << "\"}";
            return ss.str();
        }

        // Method to get the T-Matrix data field
        std::array<float, 16> getTMatrix() {
            return TMatrix;
        }

        // Method to get the Tool ID data field
        std::string getToolID() {
            return toolID;
        }
};

} // End namespace utilities
} // End namespace wosler
