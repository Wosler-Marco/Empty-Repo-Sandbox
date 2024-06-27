#include "KeyboardMouseHook.hpp"

void callback(int nCode, WPARAM wParam, LPARAM lParam) {
        if (nCode >= 0) {
            if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
                KBDLLHOOKSTRUCT* keyInfo = (KBDLLHOOKSTRUCT*)lParam;
                std::cout << keyInfo->vkCode << "\n";
            } else if (wParam == WM_LBUTTONDOWN) {
                std::cout << "Left Mouse Click" << "\n";
            } else if (wParam == WM_RBUTTONDOWN) {
                std::cout << "Right Mouse Click" << "\n";
            } else if (wParam == WM_MBUTTONDOWN) {
                std::cout << "Middle Mouse Click" << "\n";
            } else if (wParam == WM_MOUSEMOVE) {
                std::cout << "Mouse Move" << "\n";
            }
        }
}

int main(int argc, char **argv) {
    wosler::utilities::KeyboardMouseHook& instance = wosler::utilities::KeyboardMouseHook::getInstance(callback);
    std::this_thread::sleep_for(std::chrono::seconds(21600));
    std::cout << "Keyboard Mouse Hook Test stopping Module...\n";
    instance.Stop();
    return 0;
}

