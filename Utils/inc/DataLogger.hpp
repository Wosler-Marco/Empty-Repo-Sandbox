#include <iostream>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <vector>

#include "Eigen/Dense"

namespace wosler {
namespace utilities {

class Timer {
    private:
        // Private variables for the timer
        double tic;
        double toc;
        bool ticSet;
        std::chrono::time_point<std::chrono::steady_clock> zero{};

        // Private variable for a file to save the times to
        std::ofstream file;

    public:
        /**
         * @brief Construct a new Timer object
         * 
         * @param filename The name of the file to save the times to
         */
        Timer(std::string filename) {
            ticSet = false;
            file.open(filename);
        }

        /**
         * @brief Destroy the Timer object and close the file
         * 
         */
        ~Timer() {
            file.close();
        }

        /**
         * @brief Method to start the timer
         * 
         */
        void startTimer() {
            // Check to see if the timer has already been started
            if (!ticSet) {
                std::chrono::time_point<std::chrono::steady_clock> time = std::chrono::steady_clock::now();
                std::chrono::duration<double, std::milli> duration(time - zero);
                tic = duration.count();
                ticSet = true;
            }
        }

        /**
         * @brief Method to stop the timer and save the time to the file
         * 
         */
        void endTimer() {
            // Check to see if the timer was started
            if (ticSet) {
                std::chrono::time_point<std::chrono::steady_clock> time = std::chrono::steady_clock::now();
                std::chrono::duration<double, std::milli> duration(time - zero);
                toc = duration.count();
                
                double timer = toc - tic;
                file << std::to_string(timer) << "\n";
                ticSet = false;
            }
        }

        /**
         * @brief Method to stop the timer, save the time to the file, and print out the time in milliseconds on the console
         * 
         */
        void endTimerPrintoutMilliseconds() {
            // Check to see if the timer was started
            if (ticSet) {
                std::chrono::time_point<std::chrono::steady_clock> time = std::chrono::steady_clock::now();
                std::chrono::duration<double, std::milli> duration(time - zero);
                toc = duration.count();
                
                double timer = toc - tic;
                file << std::to_string(timer) << "\n";
                ticSet = false;
                
                std::cout << timer << "ms\n";
            }
        }

        /**
         * @brief Method to stop the timer, save the time to the file, and print out the time in seconds on the console
         * 
         */
        void endTimerPrintoutSeconds() {
            // Check to see if the timer was started
            if (ticSet) {
                std::chrono::time_point<std::chrono::steady_clock> time = std::chrono::steady_clock::now();
                std::chrono::duration<double, std::milli> duration(time - zero);
                toc = duration.count();
                
                double timer = toc - tic;
                file << std::to_string(timer) << "\n";
                ticSet = false;

                std::cout << std::fixed << std::setprecision(2) << timer / 1000 << "s\n";
            }
        }
};

class DataLogger {
    private:
        // Private variable for a file to save the data values to
        std::ofstream file;

    public:
        /**
         * @brief Construct a new Data Logger object
         * 
         * @param filename The name of the file to save the data values to
         */
        DataLogger(std::string filename) {
            file.open(filename);
        }

        /**
         * @brief Destroy the Data Logger object and close the file
         * 
         */
        ~DataLogger() {
            file.close();
        }

        /**
         * @brief Method to write basic data types to the file (numbers, strings, etc.)
         * 
         * @tparam T The data type
         * @param data The data to write to the file
         */
        template <typename T> void logData(const T data) {
            file << data << "\n";
        }

        /**
         * @brief Method to write basic data types to the file (numbers, strings, etc.) and print out the data to the console
         * 
         * @tparam T The data type
         * @param data The data to write to the file
         */
        template <typename T> void logDataPrintout(const T data) {
            logData(data);
            std::cout << data << "\n";
        }

        /**
         * @brief Method to write arrays to the file
         * 
         * @tparam T The array type
         * @tparam size The number of elements in the array
         * @param data The array to write to the file
         */
        template <typename T, size_t size> void logData(const T (&data)[size]) {
            std::string arrayStr = "";

            for (size_t i = 0; i < size; ++i) {
                arrayStr = arrayStr + std::to_string(data[i]) + ",";
            }
            arrayStr.pop_back();
            file << arrayStr << "\n";
        }

        /**
         * @brief Method to write arrays to the file and print out the array to the console
         * 
         * @tparam T The array type
         * @tparam size The number of elements in the array
         * @param data The array to write to the file
         */
        template <typename T, size_t size> void logDataPrintout(const T (&data)[size]) {
            logData(data);

            for (size_t i = 0; i < size; ++i) {
                std::cout << data[i] << " ";
            }
            std::cout << "\n";
        }

        /**
         * @brief Method to write vectors to the file
         * 
         * @tparam T The vector type
         * @param data The vector to write to the file
         */
        template <typename T> void logData(const std::vector<T> data) {
            std::string vectorStr = "";

            for (size_t i = 0; i < data.size(); ++i) {
                vectorStr = vectorStr + std::to_string(data.at(i)) + ",";
            }
            vectorStr.pop_back();
            file << vectorStr << "\n";
        }

        /**
         * @brief Method to write vectors to the file and print out the vector to the console
         * 
         * @tparam T The vector type
         * @param data The vector to write to the file
         */
        template <typename T> void logDataPrintout(const std::vector<T> data) {
            logData(data);

            for (size_t i = 0; i < data.size(); ++i) {
                std::cout << data.at(i) << " ";
            }
            std::cout << "\n";
        }

        /**
         * @brief Method to write T-Matrices to the file
         * 
         * @param data The T-Matrix to write to the file
         */
        void logData(const Eigen::Matrix4f data) {
            std::string matrixStr = "";

            for (size_t i = 0; i < 4; ++i) {
                for (size_t j = 0; j < 4; ++j) {
                    matrixStr = matrixStr + std::to_string(data.coeff(i, j)) + ",";
                }
            }
            matrixStr.pop_back();
            file << matrixStr << "\n";
        }

        /**
         * @brief Method to write T-Matrices to the file and print out the T-Matrix to the console
         * 
         * @param data The T-Matrix to write to the file
         */
        void logDataPrintout(const Eigen::Matrix4f data) {
            logData(data);
            std::cout << data << "\n";
        }
};

} // end namespace utilities
} // end namespace wosler
