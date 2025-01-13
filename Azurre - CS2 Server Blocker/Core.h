#pragma once

#include <Windows.h>
#include <chrono>
#include <string>
#include <thread>

#include "xor.h"
#include "Lib/imgui/imgui.h"

#define THREAD_LOOP					isRunning

#define THREAD_SLEEP(time)			std::this_thread::sleep_for(std::chrono::milliseconds(time))
#define TICK_COUNT_TIME				static_cast<float>(GetTickCount64() / 1000.f)

namespace Core {



};

inline int buildVersion = 0;
inline bool isRunning = false;
inline HWND azurre2 = nullptr;
inline WNDCLASSEX azurre2Class{};
inline ImVec2 screenSize{};