/**
 * @file SerializationInterface.hpp
 * @author Jai Sood (ankur.jai.sood@gmail.com)
 * @brief A abstract class describing a basic serialization interface
 * @version 0.1
 * @date 2022-04-15
 * 
 * @copyright Copyright (c) 2022 Wosler Inc.
 * 
 */

#pragma once

#include <iostream>

namespace wosler {
namespace utilities {

class AbstractSerializeableObject {
    private:
    const uint32_t version_number{0};

    public:
    AbstractSerializeableObject(const uint32_t version) : version_number(version) {};
    virtual ~AbstractSerializeableObject() {};

    virtual size_t Serialize(void* raw_buffer, size_t buffer_size_bytes) = 0;
    virtual bool Deserialize(const void* raw_buffer, size_t buffer_size_bytes) = 0;
};

} // end namespace utilities
} // end namepsace wosler
