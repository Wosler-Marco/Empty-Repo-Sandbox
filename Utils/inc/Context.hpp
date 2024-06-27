#pragma once

#include <iostream>
#include <filesystem>
#include <string>

namespace wosler {
namespace utilities {

class Context {
public:
    static Context& getInstance(const std::string& baseFolderName, const std::string& executablePath = std::filesystem::current_path().string()) {
        if(instance == nullptr) {
            instance = new Context(baseFolderName, executablePath);
        }
        return *instance;
    }

    Context(const Context&) = delete;
    Context&operator=(const Context&) = delete;

    static void destruct() {
        delete instance;
        instance = nullptr;
    }

    std::string getLaunchPath() const {
        return launch_path;
    }

    std::string getBasePath() const {
        return base_path;
    }

private:
    Context(const std::string& baseFolderName, const std::string& executablePath) {
        base_folder = baseFolderName;
        launch_path = executablePath;
        base_path = getBasePath(executablePath);
    };
    ~Context() {
        destruct();
    };

    std::string getBasePath(const std::string& executablePath) const {
        // Check if the path is absolute or relative
        std::filesystem::path path;
        if (std::filesystem::path(executablePath).is_absolute()) {
            // If absolute, use as is
            path = executablePath;
        } else {
            // If relative, make it absolute using the current working directory
            path = std::filesystem::absolute(executablePath);
        }
        std::filesystem::path baseDir;

        while (!path.empty() && path.filename() != base_folder) {
            path = path.parent_path();
            baseDir = path;
        }

        if (path.empty()) {
            // specified folder not found
            std::cout << "Executable not in the specified folder!\n";
            throw;
        }

        return baseDir.string();
    }

    static Context* instance;
    std::string launch_path;
    std::string base_path;
    std::string base_folder;
};

inline Context* Context::instance = nullptr;

} // end namespace wosler
} // end namespace utilities