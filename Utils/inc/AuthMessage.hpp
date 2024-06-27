#include <iostream>
#include "SerializationInterface.hpp"

#define MAX_EMAIL_SIZE 64
#define MAX_PASSWORD_SIZE 64
#define MAX_PATIENT_ID_SIZE 64

namespace wosler {
namespace utilities {

struct loginData_t {
    char username[MAX_EMAIL_SIZE] = "";
    char password[MAX_PASSWORD_SIZE] = "";
};

class loginInfo: AbstractSerializeableObject {
private:
    loginData_t data;
public:
    loginInfo(): AbstractSerializeableObject(1) {}
    loginInfo(std::string username, std::string password): AbstractSerializeableObject(1) {
        strncpy(data.username,username.c_str(),MAX_EMAIL_SIZE-1);
        strncpy(data.password,password.c_str(),MAX_PASSWORD_SIZE-1);
    }
    ~loginInfo() = default;

    std::string getUsername() {
        std::string str(data.username);
        return str;
    }

    std::string getPassword() {
        std::string str(data.password);
        return str;
    }

    void setUsername(std::string username) {
        strncpy(data.username,username.c_str(),MAX_EMAIL_SIZE-1);
        data.username[MAX_EMAIL_SIZE-1] = '\0';
    }

    void setPassword(std::string password) {
        strncpy(data.password,password.c_str(),MAX_PASSWORD_SIZE-1);
        data.password[MAX_PASSWORD_SIZE-1] = '\0';
    }
    
    virtual size_t Serialize(void *raw_buffer, size_t buffer_size_bytes) override
    {
        if (buffer_size_bytes >= sizeof(data) && raw_buffer)
        {
            // TODO: There is a reason we use memcpy here instead of std::copy(...) will write more to elaborate later
            std::memcpy(raw_buffer, &data, sizeof(data));
            return sizeof(data);
        }
        return 0;
    }

    virtual bool Deserialize(const void *raw_buffer, size_t buffer_size_bytes) override
    {
        if (buffer_size_bytes >= sizeof(data) && raw_buffer)
        {
            // TODO: There is a reason we use memcpy here instead of std::copy(...) will write more to elaborate later
            std::memcpy(&data, raw_buffer, sizeof(data));
            return true;
        }
        return false;
    }
};

struct sessionData_t {
    char patientID[MAX_PATIENT_ID_SIZE] = "";
};

class sessionInfo: AbstractSerializeableObject {
private:
    sessionData_t data;
public:
    sessionInfo(): AbstractSerializeableObject(1) {}
    sessionInfo(std::string patientID): AbstractSerializeableObject(1) {
        strncpy(data.patientID,patientID.c_str(),MAX_PATIENT_ID_SIZE-1);
    }
    ~sessionInfo() = default;

    std::string getPatientID() {
        std::string str(data.patientID);
        return str;
    }

    void setPatientID(std::string patientID) {
        strncpy(data.patientID,patientID.c_str(),MAX_PATIENT_ID_SIZE-1);
        data.patientID[MAX_PATIENT_ID_SIZE-1] = '\0';
    }
    
    virtual size_t Serialize(void *raw_buffer, size_t buffer_size_bytes) override
    {
        if (buffer_size_bytes >= sizeof(data) && raw_buffer)
        {
            // TODO: There is a reason we use memcpy here instead of std::copy(...) will write more to elaborate later
            std::memcpy(raw_buffer, &data, sizeof(data));
            return sizeof(data);
        }
        return 0;
    }

    virtual bool Deserialize(const void *raw_buffer, size_t buffer_size_bytes) override
    {
        if (buffer_size_bytes >= sizeof(data) && raw_buffer)
        {
            // TODO: There is a reason we use memcpy here instead of std::copy(...) will write more to elaborate later
            std::memcpy(&data, raw_buffer, sizeof(data));
            return true;
        }
        return false;
    }
};

} // namespace utilities
} // namespace wosler