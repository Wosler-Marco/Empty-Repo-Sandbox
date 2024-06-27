/**
 * @file KeyboardMouseHook.hpp
 * @author Jimmy Bright (jimmy.bright@wosler.ca)
 * @brief Class to grab keyboard and mouse inputs using windows hooks
 * @version 0.1
 * @date 2024-01-05
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#pragma once

#include <Windows.h>
#include <iostream>
#include "AsyncRunnable.hpp"

namespace wosler {
namespace utilities {

    struct MousePoint {
        int32_t x;
        int32_t y;

            // Overloading the assignment operator (=)
        MousePoint& operator=(const MousePoint& other) {
            if (this != &other) { // Check for self-assignment
                x = other.x;
                y = other.y;
            }
            return *this;
        }

        // Overloading the subtraction operator (-)
        MousePoint operator-(const MousePoint& other) const {
            MousePoint result;
            result.x = x - other.x;
            result.y = y - other.y;
            return result;
        }

        friend std::ostream& operator<<(std::ostream& os, const MousePoint& mp);
    };

    std::ostream& operator<<(std::ostream& os, const MousePoint& mp)
    {
        os << "(" << mp.x << "," << mp.y << ")";
        return os;
    }

    class KeyboardMouseHook : protected utilities::AsyncRunnable {
    private:

        uint64_t task_period_ms = 10;
        MSG msg;
        HHOOK keyboardHook;
        HHOOK mouseHook;
        std::function<void(int, WPARAM, LPARAM)> m_callback;

        static KeyboardMouseHook* instance; // Pointer to the instance of the class

        KeyboardMouseHook(std::function<void(int, WPARAM, LPARAM)> callback): utilities::AsyncRunnable("Ultrasound Terminal Input v2"), keyboardHook(NULL), mouseHook(NULL), m_callback(callback) {
            instance = this; // Store the instance of the class for later use
            this->Start();
        }

        void OnProcess() override {
            // Attach hooks to same thread 
            if (keyboardHook == NULL) {
                keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, StaticInputCallback, NULL, 0);
                if (keyboardHook == NULL) {
                    std::cout << "Keyboard hook installation failed.\n";
                    return;
                }
            }

            if (mouseHook == NULL) {
                mouseHook = SetWindowsHookEx(WH_MOUSE_LL, StaticInputCallback, NULL, 0);
                if (mouseHook == NULL) {
                    std::cout << "Mouse hook installation failed.\n";
                    return;
                }
            }

            if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }

        static LRESULT CALLBACK StaticInputCallback(int nCode, WPARAM wParam, LPARAM lParam) {
            // Redirect the call to the non-static member function
            if (instance) {
                return instance->inputCallback(nCode, wParam, lParam);
            }
            else {
                // Handle the case where the instance is not available
                return CallNextHookEx(NULL, nCode, wParam, lParam);
            }
        }

        LRESULT CALLBACK inputCallback(int nCode, WPARAM wParam, LPARAM lParam) {
            m_callback(nCode, wParam, lParam);
            return CallNextHookEx(NULL, nCode, wParam, lParam);
        }

    public:

        static KeyboardMouseHook& getInstance(std::function<void(int, WPARAM, LPARAM)> callback) {
            if (!instance) {
                // Create the instance only if it doesn't exist
                instance = new KeyboardMouseHook(callback);
            }
            return *instance;
        }     

        ~KeyboardMouseHook() { Stop(); }

        utilities::Error Start() {
            return RunTaskPeriodic(task_period_ms);
        }

        utilities::Error Stop() {
            if(isRunning()) {
                StopTask();
            }

            if (keyboardHook != NULL) {
                // Unhook the keyboard if it's hooked
                UnhookWindowsHookEx(keyboardHook);
                keyboardHook = NULL;
            }
            if (mouseHook != NULL) {
                // Unhook the keyboard if it's hooked
                UnhookWindowsHookEx(mouseHook);
                mouseHook = NULL;
            }

            return utilities::Error::SUCCESS;
        }
    };

    // Initialize the static member variable
    KeyboardMouseHook* KeyboardMouseHook::instance = nullptr;

} // namespace utilities
} // namespace wosler
