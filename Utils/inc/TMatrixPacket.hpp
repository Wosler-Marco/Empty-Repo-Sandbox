#include <SerializationInterface.hpp>
#include "Eigen/Dense"

namespace wosler {
    namespace utilities {
        class TmatrixPacket : public wosler::utilities::AbstractSerializeableObject {
            private:
                struct data_t {
                    uint32_t ID = 0;
                    Eigen::Matrix<float, 3, 4> Tmatrix{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}};
                    Eigen::Matrix<float, 3, 1> Forces{{0}, {0}, {0}};
                } data;

            public:
                /**
                 * @brief Construct a new T-Matrix Packet object
                 * 
                 */
                TmatrixPacket() : wosler::utilities::AbstractSerializeableObject(1) {}

                /**
                 * @brief Destroy the T-Matrix Packet object
                 * 
                 */
                ~TmatrixPacket() = default;

                /**
                 * @brief Method to print the data contained in the T-Matrix packet
                 * 
                 */
                void PrintData()
                {
                    std::cout << "ID: " << data.ID << "\nT matrix:\n" << data.Tmatrix << "\n";
                    std::cout << Eigen::Matrix<float,1,4>{{0,0,0,1}} << "\n";
                    std::cout << "Forces:\n" << data.Forces << "\n";
                }

                /**
                 * @brief Method to set the ID of the packet
                 * 
                 * @param id The ID number for the packet
                 */
                void SetID(uint32_t id) {
                    data.ID = id;
                }

                /**
                 * @brief Method to set the 4x4 T-Matrix
                 * 
                 * @param T The 4x4 T-Matrix to be stored
                 */
                void SetMatrix(Eigen::Matrix4f T) {
                    if (T.rows() == 4 && T.cols() == 4) {
                        data.Tmatrix = T(Eigen::seq(0, 2), Eigen::all);
                    } else {
                        throw std::string("Invalid matrix size");
                    }
                }

                /**
                 * @brief Method to set the forces in the X, Y, and Z direction
                 * 
                 * @param F The 3 dimensional force vector to be stored
                 */
                void SetForces(Eigen::Vector3f F) {
                    if (F.rows() == 3 && F.cols() == 1) {
                        data.Forces = F;
                    } else {
                        throw std::string("Invalid matrix size");
                    }
                }

                /**
                 * @brief Method to get the data struct from the T-Matrix Packet
                 * 
                 * @return data_t The struct containing the packet data
                 */
                data_t GetData() const { return data; }

                /**
                 * @brief Method to get the packet ID number
                 * 
                 * @return uint32_t The packet ID number
                 */
                uint32_t GetID() {
                    return data.ID;
                }

                /**
                 * @brief Method to get the 4x4 T-Matrix
                 * 
                 * @return Eigen::Matrix4f The 4x4 T-Matrix
                 */
                Eigen::Matrix4f GetMatrix() const {
                    Eigen::Matrix4f data_matrix = Eigen::Matrix4f::Zero(4, 4);
                    data_matrix.block<3, 4>(0, 0) = data.Tmatrix;
                    Eigen::Matrix<float, 1, 4> vec{{0, 0, 0, 1}};
                    data_matrix.block(3, 0, 1, 4) = vec;
                    return data_matrix;
                }

                /**
                 * @brief Method to get the 3 dimensional forces
                 * 
                 * @return Eigen::Vector3f The 3 dimensional vector containing forces in X, Y, and Z
                 */
                Eigen::Vector3f GetForces() const { return data.Forces; }

                /**
                 * @brief Method to get the size of the T-Matrix Packet
                 * 
                 * @return size_t The size of the packet
                 */
                size_t GetSize() { return sizeof(data); }

                /**
                 * @brief Method to serialize the T-Matrix Packet into a buffer
                 * 
                 * @param raw_buffer The buffer to hold the serialized packet (must be large enough to hold the data)
                 * @param buffer_size_bytes The number of bytes to store in the buffer
                 * @return size_t The size of the T-Matrix Packet if successful, otherwise zero
                 */
                virtual size_t Serialize(void *raw_buffer, size_t buffer_size_bytes) override {
                    if (buffer_size_bytes >= sizeof(data) && raw_buffer) {
                        // TODO: There is a reason we use memcpy here instead of
                        // std::copy(...) will write more to elaborate later
                        std::memcpy(raw_buffer, &data, sizeof(data));
                        return sizeof(data);
                    }
                    return 0;
                }

                /**
                 * @brief Method to deserialze the bytes to a T-Matrix Packet
                 * 
                 * @param raw_buffer The buffer containing the serialized T-Matrix Packet
                 * @param buffer_size_bytes The number of bytes to deserialize
                 * @return true Successfully deserialized to a T-Matrix Packet
                 * @return false Failed to deserialize
                 */
                virtual bool Deserialize(const void *raw_buffer, size_t buffer_size_bytes) override {
                    if (buffer_size_bytes >= sizeof(data) && raw_buffer) {
                        // TODO: There is a reason we use memcpy here instead of
                        // std::copy(...) will write more to elaborate later
                        std::memcpy(&data, raw_buffer, sizeof(data));
                        return true;
                    }
                    return false;
                }
            };
    }
}