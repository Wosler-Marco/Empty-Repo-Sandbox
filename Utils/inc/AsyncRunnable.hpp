/**
 * @file Runnable.hpp
 * @author Jai Sood (ankur.jai.sood@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2022-04-23
 * 
 * @copyright Copyright (c) 2022 Wosler Inc.
 * 
 */

#pragma once
#include <thread>

#ifndef _WIN32
#include <pthread.h>
#endif

#include <atomic>
#include <mutex>
#include <future>
#include <chrono>

#include "CommonUtilities.hpp"

namespace wosler {
namespace utilities {

class AsyncRunnable {
    private:
    std::thread runnable_thread;
    uint64_t runnable_period_ms{0};
    std::atomic<bool> runnable_running{false};
    std::future<void> runnable_future;

    void ManageFrequency() {
        #if defined(__linux__)
            pthread_setname_np(pthread_self(), name.c_str());
        #elif defined(__APPLE__) && defined(__MACH__)
            pthread_setname_np(name.c_str());
        #elif defined(_WIN32)
        #else
            #error Unknown Environment Detected!
        #endif

        std::chrono::time_point<
            std::chrono::steady_clock,
            std::chrono::steady_clock::duration
        > next_run_time;

        while(runnable_running) {
            next_run_time = std::chrono::steady_clock::now() + std::chrono::milliseconds(runnable_period_ms);

            try
            {
                OnProcess();
            }
            catch(int i)
            {
                runnable_running = false;
            }

            std::this_thread::sleep_until(next_run_time);
        }
    }

    public:
    AsyncRunnable(const std::string& runnable_name) :
        name(runnable_name) 
    {};

    virtual ~AsyncRunnable() {
        StopTask();
    };

    const std::future<void>& RunTaskOnce() {
        if(!runnable_running and !runnable_future.valid() and !runnable_thread.joinable()) {
            runnable_future = std::async(
                std::launch::async,
                [this](){
                    OnProcess();
                }
                );
        }
        return runnable_future;
    }

    Error RunTaskPeriodic(uint64_t period_ms) {
        if(!runnable_running and !runnable_thread.joinable() and !runnable_future.valid()) {
            runnable_running = true;
            runnable_period_ms = period_ms;
            runnable_thread = std::thread(&AsyncRunnable::ManageFrequency, this);
            return Error::SUCCESS;
        }
        return Error::INVALID_STATE;
    }

    Error StopTask() {
        if(runnable_future.valid()) {
            runnable_future.wait();
            runnable_future.get();
        }
        if(runnable_thread.joinable()) {
            runnable_running = false;
            runnable_thread.join();
        }
        return Error::SUCCESS;
    }

    bool isRunning() {
        return runnable_running;
    }

    protected:
    const std::string name{""};
    virtual void OnProcess() = 0;
};

} // end namespace utilities
} // end namespace wosler
