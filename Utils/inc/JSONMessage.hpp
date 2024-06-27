#include <iostream>
#include <vector>
#include <chrono>
#include <sstream>
#include "JSON/json.hpp"
#include "TMatrixPacket.hpp"

namespace wosler {
namespace utilities {

// Abstract class to create a base message object
class message {
    protected:
        // Metatdata variables that will be contained in all messages
        std::uint32_t messageID;
        std::uint32_t commandID;
        std::time_t UTCTime;
        std::uint32_t errorCode;

    public:
        /**
         * @brief Method to print the data contained in the message
         * 
         */
        virtual void printMessage() = 0;
        
        /**
         * @brief Get the Message ID field
         * 
         * @return std::uint32_t The Message ID
         */
        std::uint32_t getMessageID() {
            return messageID;
        }

        /**
         * @brief Get the Command ID field
         * 
         * @return std::uint32_t The Command ID
         */
        std::uint32_t getCommandID() {
            return commandID;
        }

        /**
         * @brief Get the UTC Time field
         * 
         * @return std::time_t The UTC time
         */
        std::time_t getUTCTime() {
            return UTCTime;
        }

        /**
         * @brief Get the Error Code field
         * 
         * @return std::uint32_t The error code
         */
        std::uint32_t getErrorCode() {
            return errorCode;
        }
};

class authenticationMessage: public message {
    private:
        // Variables specific to the Authentication Message
        std::string clinicID;
        std::string roboticArm;
        std::string roomID;

    public:
        /**
         * @brief Construct a new empty Authentication Message object (Allows the nlohmann::from_json method to work)
         * 
         */
        authenticationMessage() = default;

        /**
         * @brief Construct a new Authentication Message object
         * 
         * @param message A unique Message ID
         * @param clinic The clinic name
         * @param robot The robotic arm number in the clinic
         * @param room The room number in the clinic
         * @param error An error code
         */
        authenticationMessage(std::uint32_t message, std::string clinic, std::string robot, std::string room, std::uint32_t error) {
            messageID = message;
            commandID = 1093677105;
            UTCTime = std::time(0);
            clinicID = clinic;
            roboticArm = robot;
            roomID = room;
            errorCode = error;
        }

        /**
         * @brief Method to convert a JSON object to a Authentication Message
         * 
         * @param nlohmann_json_j The JSON object containing all of the key-value pairs in a Authentication Message
         * @param nlohmann_json_t A empty Authentication Message
         */
        friend void from_json(const nlohmann::json& nlohmann_json_j, authenticationMessage& nlohmann_json_t) {
            nlohmann_json_t.messageID = nlohmann_json_j.at("messageID");
            nlohmann_json_t.commandID = nlohmann_json_j.at("commandID");
            nlohmann_json_t.UTCTime = nlohmann_json_j.at("UTCTime");
            nlohmann_json_t.clinicID = nlohmann_json_j.at("clinic");
            nlohmann_json_t.roboticArm = nlohmann_json_j.at("roboticArm");
            nlohmann_json_t.roomID = nlohmann_json_j.at("room");
            nlohmann_json_t.errorCode = nlohmann_json_j.at("errorCode");
        }

        /**
         * @brief Method to convert a Authentication Message to a JSON object
         * 
         * @param nlohmann_json_j A empty JSON object
         * @param nlohmann_json_t A Authentication Message containing all of the data to be stored in the JSON object
         */
        friend void to_json(nlohmann::json& nlohmann_json_j, const authenticationMessage& nlohmann_json_t) {
            nlohmann_json_j = nlohmann::json{ {"messageID", nlohmann_json_t.messageID}, {"commandID", nlohmann_json_t.commandID}, {"UTCTime", nlohmann_json_t.UTCTime}, {"clinic", nlohmann_json_t.clinicID}, {"roboticArm", nlohmann_json_t.roboticArm}, {"room", nlohmann_json_t.roomID}, {"errorCode", nlohmann_json_t.errorCode} };
        }

        /**
         * @brief Method to print the data contained in the message
         * 
         */
        void printMessage() {
            std::cout << "Message ID: " << messageID << "\n" << "Command ID: " << commandID << "\n" << "UTC Time: " << asctime(gmtime(&UTCTime)) << "Clinic ID: " << clinicID << "\n" << "Robotic Arm: " << roboticArm << "\n"  << "Room ID: " << roomID << "\n"<< "Error Code: " << errorCode << "\n\n";
        }

        /**
         * @brief Get the Clinic ID field
         * 
         * @return std::string The clinic name
         */
        std::string getClinicID() {
            return clinicID;
        }

        /**
         * @brief Get the Robotic Arm field
         * 
         * @return std::string The robotic arm number in the clinic
         */
        std::string getRoboticArm() {
            return roboticArm;
        }

        /**
         * @brief Get the Room ID field
         * 
         * @return std::string The room number in the clinic
         */
        std::string getRoomID() {
            return roomID;
        }
};

class controlMessage: public message {
    private:
        // Variable specific to the Control Message
        std::vector<char> packet;
    
    public:
        /**
         * @brief Construct a new empty Control Message object (Allows the nlohmann::from_json method to work)
         * 
         */
        controlMessage() = default;

        /**
         * @brief Construct a new Control Message object
         * 
         * @param message A unique Message ID
         * @param matrix A serialized T-Matrix packet
         * @param error An error code
         */
        controlMessage(std::uint32_t message, TmatrixPacket& matrix, std::uint32_t error) {
            messageID = message;
            commandID = 1093677106;
            UTCTime = std::time(0);

            packet.resize(sizeof(TmatrixPacket));
            matrix.Serialize(packet.data(), sizeof(TmatrixPacket));

            errorCode = error;
        }

        /**
         * @brief Method to convert a JSON object to a Control Message
         * 
         * @param nlohmann_json_j The JSON object containing all of the key-value pairs in a Control Message
         * @param nlohmann_json_t A empty Control Message
         */
        friend void from_json(const nlohmann::json& nlohmann_json_j, controlMessage& nlohmann_json_t) {
            nlohmann_json_t.messageID = nlohmann_json_j.at("messageID");
            nlohmann_json_t.commandID = nlohmann_json_j.at("commandID");
            nlohmann_json_t.UTCTime = nlohmann_json_j.at("UTCTime");

            nlohmann_json_t.packet.resize(sizeof(TmatrixPacket));
            nlohmann_json_t.packet = nlohmann_json_j.at("packet");

            nlohmann_json_t.errorCode = nlohmann_json_j.at("errorCode");
        }

        /**
         * @brief Method to convert a Control Message to a JSON object
         * 
         * @param nlohmann_json_j A empty JSON object
         * @param nlohmann_json_t A Control Message containing all of the data to be stored in the JSON object
         */
        friend void to_json(nlohmann::json& nlohmann_json_j, const controlMessage& nlohmann_json_t) {
            nlohmann_json_j = nlohmann::json{ {"messageID", nlohmann_json_t.messageID}, {"commandID", nlohmann_json_t.commandID}, {"UTCTime", nlohmann_json_t.UTCTime}, {"packet", nlohmann_json_t.packet}, {"errorCode", nlohmann_json_t.errorCode} };
        }

        /**
         * @brief Method to print the data contained in the message
         * 
         */
        void printMessage() {
            std::cout << "Message ID: " << messageID << "\n" << "Command ID: " << commandID << "\n" << "UTC Time: " << asctime(gmtime(&UTCTime)) << "Error Code: " << errorCode << "\n";

            TmatrixPacket pack;
            pack.Deserialize(packet.data(), sizeof(TmatrixPacket));
            pack.PrintData();

            std::cout << "\n";
        }
        
        /**
         * @brief Get the serialized T-Matrix packet
         * 
         * @return std::vector<char> The serialized T-Matrix packet
         */
        std::vector<char> getPacket() {
            return packet;
        }
};

class toolMessage: public message {
    private:
        // Variable specific to the Tool Message
        std::string toolID;

    public:
        /**
         * @brief Construct a new empty Tool Message object (Allows the nlohmann::from_json method to work)
         * 
         */
        toolMessage() = default;

        /**
         * @brief Construct a new Tool Message object
         * 
         * @param message A unique Message ID
         * @param tool The tool to switch to
         * @param error An error code
         */
        toolMessage(std::uint32_t message, std::string tool, std::uint32_t error) {
            messageID = message;
            commandID = 1093677107;
            UTCTime = std::time(0);
            toolID = tool;
            errorCode = error;
        }

        /**
         * @brief Method to convert a JSON object to a Tool Message
         * 
         * @param nlohmann_json_j The JSON object containing all of the key-value pairs in a Tool Message
         * @param nlohmann_json_t A empty Tool Message
         */
        friend void from_json(const nlohmann::json& nlohmann_json_j, toolMessage& nlohmann_json_t) {
            nlohmann_json_t.messageID = nlohmann_json_j.at("messageID");
            nlohmann_json_t.commandID = nlohmann_json_j.at("commandID");
            nlohmann_json_t.UTCTime = nlohmann_json_j.at("UTCTime");
            nlohmann_json_t.toolID = nlohmann_json_j.at("tool");
            nlohmann_json_t.errorCode = nlohmann_json_j.at("errorCode");
        }

        /**
         * @brief Method to convert a Tool Message to a JSON object
         * 
         * @param nlohmann_json_j A empty JSON object
         * @param nlohmann_json_t A Tool Message containing all of the data to be stored in the JSON object
         */
        friend void to_json(nlohmann::json& nlohmann_json_j, const toolMessage& nlohmann_json_t) {
            nlohmann_json_j = nlohmann::json{ {"messageID", nlohmann_json_t.messageID}, {"commandID", nlohmann_json_t.commandID}, {"UTCTime", nlohmann_json_t.UTCTime}, {"tool", nlohmann_json_t.toolID}, {"errorCode", nlohmann_json_t.errorCode} };
        }

        /**
         * @brief Method to print the data contained in the message
         * 
         */
        void printMessage() {
            std::cout << "Message ID: " << messageID << "\n" << "Command ID: " << commandID << "\n" << "UTC Time: " << asctime(gmtime(&UTCTime)) << "Tool ID: " << toolID << "\n" << "Error Code: " << errorCode << "\n\n";
        }

        /**
         * @brief Get the Tool ID field
         * 
         * @return std::string The tool to switch to
         */
        std::string getToolID() {
            return toolID;
        }
};

class gelDispenseMessage: public message {
    private:
        // Variable specific to the Gel Dispense Message
        std::string amount;

    public:
        /**
         * @brief Construct a new empty Gel Dispense Message object (Allows the nlohmann::from_json method to work)
         * 
         */
        gelDispenseMessage() = default;

        /**
         * @brief Construct a new Gel Dispense Message object
         * 
         * @param message A unique Message ID
         * @param gel The amount of gel to dispense
         * @param error An error code
         */
        gelDispenseMessage(std::uint32_t message, std::string gel, std::uint32_t error) {
            messageID = message;
            commandID = 1093677108;
            UTCTime = std::time(0);
            amount = gel;
            errorCode = error;
        }

        /**
         * @brief Method to convert a JSON object to a Gel Dispense Message
         * 
         * @param nlohmann_json_j The JSON object containing all of the key-value pairs in a Gel Dispense Message
         * @param nlohmann_json_t A empty Gel Dispense Message
         */
        friend void from_json(const nlohmann::json& nlohmann_json_j, gelDispenseMessage& nlohmann_json_t) {
            nlohmann_json_t.messageID = nlohmann_json_j.at("messageID");
            nlohmann_json_t.commandID = nlohmann_json_j.at("commandID");
            nlohmann_json_t.UTCTime = nlohmann_json_j.at("UTCTime");
            nlohmann_json_t.amount = nlohmann_json_j.at("amount");
            nlohmann_json_t.errorCode = nlohmann_json_j.at("errorCode");
        }

        /**
         * @brief Method to convert a Gel Dispense Message to a JSON object
         * 
         * @param nlohmann_json_j A empty JSON object
         * @param nlohmann_json_t A Gel Dispense Message containing all of the data to be stored in the JSON object
         */
        friend void to_json(nlohmann::json& nlohmann_json_j, const gelDispenseMessage& nlohmann_json_t) {
            nlohmann_json_j = nlohmann::json{ {"messageID", nlohmann_json_t.messageID}, {"commandID", nlohmann_json_t.commandID}, {"UTCTime", nlohmann_json_t.UTCTime}, {"amount", nlohmann_json_t.amount}, {"errorCode", nlohmann_json_t.errorCode} };
        }

        /**
         * @brief Method to print the data contained in the message
         * 
         */
        void printMessage() {
            std::cout << "Message ID: " << messageID << "\n" << "Command ID: " << commandID << "\n" << "UTC Time: " << asctime(gmtime(&UTCTime)) << "Amount of gel: " << amount << "\n" << "Error Code: " << errorCode << "\n\n";
        }

        /**
         * @brief Get the Gel Amount field
         * 
         * @return std::string The amount of gel to dispense
         */
        std::string getAmount() {
            return amount;
        }
};

class homingMessage: public message {
    private:
        // Variables specific to the Homing Message
        std::vector<char> packet;
        std::string toolID;
    
    public:
        /**
         * @brief Construct a new empty Homing Message object (Allows the nholmann::from_json method to work)
         * 
         */
        homingMessage() = default;

        /**
         * @brief Construct a new Homing Message object
         * 
         * @param message A unique Message ID
         * @param matrix A serialized T-Matrix packet
         * @param tool The tool to switch to
         * @param error An error code
         */
        homingMessage(std::uint32_t message, TmatrixPacket& matrix, std::string tool, std::uint32_t error) {
            messageID = message;
            commandID = 1093677109;
            UTCTime = std::time(0);
            
            packet.resize(sizeof(TmatrixPacket));
            matrix.Serialize(packet.data(), sizeof(TmatrixPacket));

            toolID = tool;
            errorCode = error;
        }

        /**
         * @brief Method to convert a JSON object to a Homing Message
         * 
         * @param nlohmann_json_j The JSON object containing all of the key-value pairs in a Homing Message
         * @param nlohmann_json_t An empty Homing Message
         */
        friend void from_json(const nlohmann::json& nlohmann_json_j, homingMessage& nlohmann_json_t) {
            nlohmann_json_t.messageID = nlohmann_json_j.at("messageID");
            nlohmann_json_t.commandID = nlohmann_json_j.at("commandID");
            nlohmann_json_t.UTCTime = nlohmann_json_j.at("UTCTime");

            nlohmann_json_t.packet.resize(sizeof(TmatrixPacket));
            nlohmann_json_t.packet = nlohmann_json_j.at("packet");

            nlohmann_json_t.toolID = nlohmann_json_j.at("tool");
            nlohmann_json_t.errorCode = nlohmann_json_j.at("errorCode");
        }

        /**
         * @brief Method to convert a Homing Message to a JSON object
         * 
         * @param nlohmann_json_j An empty JSON object
         * @param nlohmann_json_t A Homing Message containing all of the data to be stored in the JSON object
         */
        friend void to_json(nlohmann::json& nlohmann_json_j, const homingMessage& nlohmann_json_t) {
            nlohmann_json_j = { {"messageID", nlohmann_json_t.messageID}, {"commandID", nlohmann_json_t.commandID}, {"UTCTime", nlohmann_json_t.UTCTime}, {"packet", nlohmann_json_t.packet}, {"tool", nlohmann_json_t.toolID}, {"errorCode", nlohmann_json_t.errorCode} };
        }

        /**
         * @brief Method to print the data contained in the message
         * 
         */
        void printMessage() {
            std::cout << "Message ID: " << messageID << "\n" << "Command ID: " << commandID << "\n" << "UTC Time: " << asctime(gmtime(&UTCTime)) << "Tool ID: " << toolID << "\n" << "Error Code: " << errorCode << "\n";

            TmatrixPacket pack;
            pack.Deserialize(packet.data(), sizeof(TmatrixPacket));
            pack.PrintData();

            std::cout << "\n";
        }

        /**
         * @brief Get the serialized T-Matrix packet
         * 
         * @return std::vector<char> The serialized T-Matrix packet
         */
        std::vector<char> getPacket() {
            return packet;
        }

        /**
         * @brief Get the Tool ID field
         * 
         * @return std::string The tool to switch to
         */
        std::string getToolID() {
            return toolID;
        }
};

} // End namespace utilities
} // End namespace wosler
