#pragma once

#include <Windows.h>
#include <chrono>
#include <string>
#include <thread>
#include <fstream>

#include "Relay.h"
#include "xor.h"
#include "Lib/imgui/imgui.h"

#define THREAD_LOOP					isRunning

#define __MONTH__ (\
    __DATE__[2] == 'n' ? (__DATE__[1] == 'a' ? "01" : "06") \
    : __DATE__[2] == 'b' ? "02" \
    : __DATE__[2] == 'r' ? (__DATE__[0] == 'M' ? "03" : "04") \
    : __DATE__[2] == 'y' ? "05" \
    : __DATE__[2] == 'l' ? "07" \
    : __DATE__[2] == 'g' ? "08" \
    : __DATE__[2] == 'p' ? "09" \
    : __DATE__[2] == 't' ? "10" \
    : __DATE__[2] == 'v' ? "11" \
    : "12")

#define THREAD_SLEEP(time)			std::this_thread::sleep_for(std::chrono::milliseconds(time))

namespace Core {
    void init() noexcept;
    void sortAlfa(std::vector<Relay>& relays);
    void sortCity(std::vector<Relay>& relays);
    bool updateSDRConfig() noexcept;

    constexpr std::string getVersion() noexcept {
        const std::string date = __DATE__;
        std::string v = date.substr(9, 10);
        v += __MONTH__;
        v += static_cast<char>(date[4]) != ' ' ? date[4] : '0';
        v += date[5];
        return v;
    }
};

inline std::vector<Relay> relays;
inline int buildVersion = 0;
inline bool isRunning = false;
inline HWND azurre2 = nullptr;
inline WNDCLASSEX azurre2Class{};
inline ImVec2 screenSize = { 640.f, 480.f };
constexpr const char* url = "https://api.steampowered.com/ISteamApps/GetSDRConfig/v1/?appid=730";