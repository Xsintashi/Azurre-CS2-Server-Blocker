#pragma once

#include <windows.h>

namespace GUI {
    void init() noexcept;
    void create() noexcept;
    void render() noexcept;
    void destroy() noexcept;
    void applyBlur(HWND hwnd, bool enable);
}