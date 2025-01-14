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

#define THREAD_SLEEP(time)			std::this_thread::sleep_for(std::chrono::milliseconds(time))
#define TICK_COUNT_TIME				static_cast<float>(GetTickCount64() / 1000.f)

namespace Core {
    void init() noexcept;
    void sortAlfa(std::vector<Relay>& relays);
    void sortCity(std::vector<Relay>& relays);
    bool updateSDRConfig() noexcept;

};

inline std::vector<Relay> relays;
inline int buildVersion = 0;
inline bool isRunning = false;
inline HWND azurre2 = nullptr;
inline WNDCLASSEX azurre2Class{};
inline ImVec2 screenSize = { 640.f, 480.f };
constexpr const char* url = "https://api.steampowered.com/ISteamApps/GetSDRConfig/v1/?appid=730";