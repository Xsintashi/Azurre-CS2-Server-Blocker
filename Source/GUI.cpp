#define IMGUI_DEFINE_MATH_OPERATORS
#include "../Lib/ImGui/imgui.h"
#include "../Lib/ImGui/imgui_impl_dx11.h"
#include "../Lib/ImGui/imgui_impl_win32.h"
#include "../Lib/imgui/imgui_stdlib.h"
#include "../Lib/ImGui/ImGuiCustom.h"
#include "resource.h"
#include <bitset>
#include <d3d11.h>
#include <dwmapi.h>
#include <fstream>
#include <iostream>
#include <ShlObj.h>
#include <string>
#include <algorithm>

#include "GUI.h"
#include "Core.h"
#include "Lock.h"
#include "Firewall.h"

#pragma execution_character_set("utf-8")
// Data
static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
static IDXGISwapChain* g_pSwapChain = nullptr;
static UINT g_ResizeWidth = 0, g_ResizeHeight = 0;
static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;
static ID3D11Device* g_pd3dDevice = nullptr;

constexpr float childWidth = 240.f;
constexpr int menuWidth = 576;
constexpr int menuHeight = 512;

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static void getColors() {
    const auto& colors = ImGui::GetStyle().Colors;
    std::ostringstream ss;

    for (int i = 0; i < ImGuiCol_COUNT; ++i)
    {
        ss << "colors[ImGuiCol_" << ImGui::GetStyleColorName(i) << "] = ImVec4(";
        ss.precision(2);
        ss << std::fixed << colors[i].x;
        ss << "f, ";
        ss << colors[i].y;
        ss << "f, ";
        ss << colors[i].z;
        ss << "f, ";
        ss << colors[i].w;
        ss << "f);\n";
    }
    ImGui::SetClipboardText(ss.str().c_str());
}

void GUI::applyBlur(HWND hwnd, bool enable) {

    enum AccentState {
        DISABLED = 0,
        GRADIENT, // Window painted completely with the accent color
        TRANSPARENTGRADIENT, // Window painted completely with the accent color
        BLURBEHIND // Blur 
    };

    struct ACCENTPOLICY {
        AccentState accentState;
        int accentFlags;
        int gradientColor;
        int animationId;
    };
    struct WINCOMPATTRDATA {
        int na;
        PVOID pd;
        ULONG ul;
    };

    const HINSTANCE user32 = LoadLibrary("user32.dll");
    if (user32) {
        typedef BOOL(WINAPI* pSetWindowCompositionAttribute)(HWND, WINCOMPATTRDATA*);

        const pSetWindowCompositionAttribute SetWindowCompositionAttribute = (pSetWindowCompositionAttribute)GetProcAddress(user32, "SetWindowCompositionAttribute");
        if (SetWindowCompositionAttribute) {
            ACCENTPOLICY policy = { enable ? AccentState::BLURBEHIND : AccentState::DISABLED , 0, 0, 0 };
            WINCOMPATTRDATA data = { 19, &policy,sizeof(ACCENTPOLICY) };
            SetWindowCompositionAttribute(hwnd, &data);
        }
        FreeLibrary(user32);
    }
}

constexpr const char* weirdTitles[] = {
    ("What you see is what you get!"),
    ("Your Third Eye!"),
    ("VAC-Proof"),
    ("Valve Azurre Cheat"),
    ("nothingplacetypesimply"),
    ("Your CS rating, Your Auschwitz number."),
    ("void (*(*azurre[])()) ();"),
    ("Warning: Failed to respond to a message. This is a memory leak."),
    ("Side effects may include: dry mouth, nausea, vomiting, water retention, painful rectal itch, hallucination, dementia, psychosis, coma, death, and halitosis. Magic is not for everyone. Consult your doctor before use"),
    ("Lobotomy."),
    (" "),
    ("1"),
    ("hvh?"),
    ("HvH"),
    ("HvH"),
    ("Safest place for u to stand is in front of your enemy's gun..."),
    ("bvfndsubmdsj vudsa,vsjnfn   ., .,.,"),
    ("Shmubbaaaaaaaaaa"),
    ("Azurre AntiVirus"),
    ("Azurre Anti Cheat"),
    ("Mouse is optional."),
    ("Monitor is optional."),
    ("-insecure"),
    ("-allow_third_party_software"),
    ("cl_junkcode 1"),
    ("sv_junkcode 1"),
    ("sv_cheats 1"),
    ("powered by a tree!"),
    ("You can't cheat on VAC Secured Servers!!!"),
    ("we smokin penises in fights"),
    ("從頭部 - хорошо，從後面 - еще lyчше。"),
    ("我們打架時抽煙."),
    (":D"),
    ("0x0000000000000000"),
    ("0xFFFFFFFFFFFFFFFF"),
    ("0xCCCCCCCCCCCCCCCC"),
    ("shrek pfp shit cwazy"),
    ("KEEP GOIN HARD ON 500HZ PANELS ARE SCHIZOPHRENIC."),
    ("Is more bing chilling..."),
    ("you lagging across ur pc should restart pc was like sweaty booty cheeks"),
    ("conflictinganitchitanduser"),
    ("I see... absolutely nothing."),
    ("You have been banned from playing on secure servers due to an infraction by this or a linked account."),
    ("NEVER SHOW AGAIN"),
    ("亲爱的玩家：为响应相关通知，CS2将于2034年06月6日（周二）3:37 – 3:99停服一天，感谢各位玩家的支持。"),
    ("2023 Service Medal"),
    ("Go To Scanner"),
    ("Visual last night"),
    ("Two Years later, Valve has patched the critical remote code exceution exploit."),
    ("Heading for the source code"),
    ("No information given. Do you need them?"),
    ("Service Medal > Account"),
    ("Look for an e-girl on Agency"),
    ("Know the rules before breaking them"),
    ("WILL NEVER BE BANNED"),
    ("In Overwatch Session"),
    ("Boosters > Cheaters"),
    ("Fatty Valve Employes when FSM Farm"),
    ("Bypassing VAC on Office"),
    ("ススススススススススス。。。"),
    ("Bruh tell me why dragon flies look like baby helicopters"),
    ("straight tongue ksising men idc"),
};

void GUI::create() noexcept {
    const int x = static_cast<int>(screenSize.x);
    const int y = static_cast<int>(screenSize.y);

    azurre2Class.cbSize = sizeof(WNDCLASSEX);
    azurre2Class.style = CS_CLASSDC;
    azurre2Class.lpfnWndProc = WndProc;
    azurre2Class.cbClsExtra = 0;
    azurre2Class.cbWndExtra = 0;
    azurre2Class.hInstance = 0;
    azurre2Class.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_RED));
    azurre2Class.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_RED));
    azurre2Class.hCursor = LoadCursor(hInstance, IDC_CROSS);
    azurre2Class.hbrBackground = 0;
    azurre2Class.lpszMenuName = 0;
    azurre2Class.lpszClassName = "azurre";

    RegisterClassEx(&azurre2Class);
    srand((uint32_t)time(0));
    azurre2 = CreateWindowEx(
        0,
        "azurre",
        weirdTitles[rand() % IM_ARRAYSIZE(weirdTitles)],
        WS_POPUP | WS_VISIBLE,
        0,
        0,
        x,
        y,
        0,
        0,
        azurre2Class.hInstance,
        0
    );

    SetLayeredWindowAttributes(azurre2, RGB(0, 0, 0), BYTE(255), LWA_ALPHA);

    // Initialize Direct3D
    if (!CreateDeviceD3D(azurre2))
    {
        CleanupDeviceD3D();
        ::UnregisterClassA(azurre2Class.lpszClassName, azurre2Class.hInstance);
        return;
    }

    RECT clientArea{};
    GetClientRect(azurre2, &clientArea);
    RECT windowArea{};
    GetWindowRect(azurre2, &windowArea);
    POINT diff{};
    ClientToScreen(azurre2, &diff);

    const MARGINS margins{
    windowArea.left + (diff.x - windowArea.left),
        windowArea.top + (diff.y - windowArea.top),
        clientArea.right,
        clientArea.bottom
    };
    DwmExtendFrameIntoClientArea(azurre2, &margins);

    // Show the window
    ShowWindow(azurre2, SW_SHOW);
    SetWindowTheme(azurre2, L"", L"");
    //SetWindowPos(azurre2, HWND_TOPMOST, windowArea.left, windowArea.top, static_cast<int>(screenSize.x), static_cast<int>(screenSize.y), SWP_NOMOVE | SWP_NOSIZE);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    //io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;       // Enable Docking
    //io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;     // Enable Multi-Viewport / Platform Windows
    io.IniFilename = NULL;
    ImGui::StyleColorsGlass();
    ImGuiStyle& style = ImGui::GetStyle();
    style.SeparatorTextAlign.x = 0.5f;
    ImGui_ImplWin32_Init(azurre2);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);
}

void GUI::render() noexcept {
    POINTS pos;
    applyBlur(azurre2, true);
    static DWORD sleepTime = 1;
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    while (THREAD_LOOP) {
        // Poll and handle messages (inputs, window resize, etc.)
        // See the WndProc() function below for our to dispatch events to the Win32 backend.
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            switch (msg.message) {
                case WM_QUIT: {
                    isRunning = false;
                    return;
                }
                case WM_LBUTTONDOWN: {
                    pos = MAKEPOINTS(msg.lParam); // set click points
                    break;
                }
                case WM_MOUSEMOVE: {
                    if (msg.wParam == MK_LBUTTON)
                    {
                        const auto points = MAKEPOINTS(msg.lParam);
                        auto rect = ::RECT{ };

                        GetWindowRect(azurre2, &rect);

                        rect.left += points.x - pos.x;
                        rect.top += points.y - pos.y;

                        if (pos.x >= 0 &&
                            pos.x <= screenSize.x &&
                            pos.y >= 0 && pos.y <= 19)
                            SetWindowPos(
                                azurre2,
                                HWND_TOPMOST,
                                rect.left,
                                rect.top,
                                0, 0,
                                SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOZORDER
                            );
                    }
                    break;
                }
            }

        }
        if (GetActiveWindow() != azurre2) {
            THREAD_SLEEP(50);
            continue;
        }
        // Handle window resize (we don't resize directly in the WM_SIZE handler)
        if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
            g_ResizeWidth = g_ResizeHeight = 0;
            CreateRenderTarget();
        }

        // Start the Dear ImGui frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        {
            ImGui::SetNextWindowPos({ 0, 0 });
            ImGui::SetNextWindowSize(screenSize);
            ImGui::Begin("Azurre - CS2 Server Blocker", &isRunning, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoShadows);
            ImGui::BeginDisabled(busy != BUSYMODE::NOT);
            if (ImGui::Button("Sort alphabetically"))
                Core::sortAlfa(relays);
            ImGui::SameLine();

            if (ImGui::Button("Sort by Country"))
                Core::sortCity(relays);
            ImGui::SameLine();
            if (ImGui::Button("Block All"))
                std::thread(Core::blockUnBlockRelays, true).detach();
            ImGui::SameLine();

            if (ImGui::Button("Unblock All"))
                std::thread(Core::blockUnBlockRelays, false).detach();
            ImGui::SameLine();

            ImGui::SameLine();
            ImGui::SetCursorPosX(screenSize.x - 66.f);
            if (ImGui::Button("Refresh"))
                Core::init();
            static std::string search;
            ImGui::InputTextWithHint("##search", "Search Server",&search, ImGuiInputTextFlags_CharsLowercase);
            std::transform(search.begin(), search.end(), search.begin(), [](unsigned char c) { return std::tolower(c); });
            ImGui::SeparatorText("Servers");
            ImGui::BeginChild("##servers", { -1, -12 }, false, 0);
            int blockedAmount = 0;
            if (relays.empty()) 
                ImGui::Text("No Server found :(!");
            else {
                for (auto& relay : relays) {
                    std::string relayName = relay.name;
                    std::transform(relayName.begin(), relayName.end(), relayName.begin(), [](unsigned char c) { return std::tolower(c); });
                    if (!search.empty() && relayName.find(search) == std::string::npos)
                        continue;
                    ImGui::PushID(relay.code.c_str());
                    ImGui::PushStyleColor(ImGuiCol_Text, relay.blocked ? ImVec4{1.f, 0.f, 0.f, 1.f} : ImVec4{ 0.f, 1.f, 0.f, 1.f });
                    if (ImGui::Button(relay.name.c_str(), { -1, 32 }))
                        relay.blocked = relay.blocked ? Firewall::unblockRelay(relay) : !Firewall::blockRelay(relay);

                    if (relay.blocked)
                        blockedAmount++;

                    ImGui::PopStyleColor();
                    ImGui::PopID();
                }
            }
            ImGui::EndChild();
            ImGui::EndDisabled();
            ImGui::Text("Found %d servers.", relays.size());
            ImGui::SameLine();
            ImGui::SetCursorPosX(screenSize.x - 59.f);
            ImGui::Text("v%s", Core::getVersion().c_str());
            ImGui::End();
            Core::taskbarAnimation((int)relays.size(), blockedAmount);
        }
        // Rendering
        ImGui::Render();
        constexpr float alpha[4] = { 0.f, 0.f, 0.f, 0.f };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        // Update and Render additional Platform Windows
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }

        g_pSwapChain->Present(1, 0); // Present with vsync
        //g_pSwapChain->Present(0, 0); // Present without vsync
    }
}

void GUI::destroy() noexcept {
    // Cleanup
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(azurre2);
    ::UnregisterClassA(azurre2Class.lpszClassName, azurre2Class.hInstance);
}

void GUI::init() noexcept {
    THREAD_LOOP = true;
    GUI::create();
    GUI::render();
    GUI::destroy();
}

// Helper functions
bool CreateDeviceD3D(HWND hWnd)
{
    // Setup swap chain
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    //createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res == DXGI_ERROR_UNSUPPORTED) // Try high-performance WARP software driver if hardware is not available.
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

#ifndef WM_DPICHANGED
#define WM_DPICHANGED 0x02E0 // From Windows SDK 8.1+ headers
#endif

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED)
            return 0;
        g_ResizeWidth = (UINT)LOWORD(lParam); // Queue resize
        g_ResizeHeight = (UINT)HIWORD(lParam);
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    case WM_DPICHANGED:
        if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_DpiEnableScaleViewports)
        {
            //const int dpi = HIWORD(wParam);
            //printf("WM_DPICHANGED to %d (%.0f%%)\n", dpi, (float)dpi / 96.0f * 100.0f);
            const RECT* suggested_rect = (RECT*)lParam;
            ::SetWindowPos(hWnd, nullptr, suggested_rect->left, suggested_rect->top, suggested_rect->right - suggested_rect->left, suggested_rect->bottom - suggested_rect->top, SWP_NOZORDER | SWP_NOACTIVATE);
        }
        break;
    }
    return ::DefWindowProc(hWnd, msg, wParam, lParam);
}
