#include "Core.h"
#include "Firewall.h"
#include "resource.h"
#include "../Lib/nlohmann/json.hpp"
#include <ShlObj.h>

using json = nlohmann::json;
json sdr{};

void setIcon(unsigned long ID) noexcept {
    SendMessage(azurre2, WM_SETICON, ICON_BIG, (LPARAM)LoadIcon(hInstance, MAKEINTRESOURCE(ID)));
    SendMessage(azurre2, WM_SETICON, ICON_SMALL, (LPARAM)LoadIcon(hInstance, MAKEINTRESOURCE(ID)));
}

void setTaskbarProgressColor(int progressValue, int progressMax, TBPFLAG progressState) {
    HRESULT hr = CoInitialize(NULL);
    if (FAILED(hr))
        return;

    ITaskbarList3* pTaskbar = nullptr;
    hr = CoCreateInstance(CLSID_TaskbarList, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pTaskbar));
    if (SUCCEEDED(hr)) {
        hr = pTaskbar->HrInit();
        if (SUCCEEDED(hr)) {
            if (progressMax > 0) {
                pTaskbar->SetProgressValue(azurre2, progressValue, progressMax);
                pTaskbar->SetProgressState(azurre2, progressState);
            } else 
                pTaskbar->SetProgressState(azurre2, TBPF_NOPROGRESS);
        }
        pTaskbar->Release();
    }

    CoUninitialize();
}

void Core::taskbarAnimation(int relays, int blocked) noexcept {
    static int cached[2] = {0, 0};

    if (relays == cached[0] && blocked == cached[1])
        return;

    cached[0] = relays;
    cached[1] = blocked;

    constexpr int stages = 3; // Green -> Yellow -> Red
    int fractor = relays / stages;
    int blockedFractor = blocked/ fractor;
    setTaskbarProgressColor(blocked, relays, (blockedFractor ? (blockedFractor == 1 ? TBPF_PAUSED : TBPF_ERROR) : TBPF_NORMAL));
    setIcon((blockedFractor ? (blockedFractor == 1 ? IDI_YELLOW : IDI_RED) : IDI_GREEN));

}

void Core::blockUnBlockRelays(bool block) {
    busy = BUSYMODE::MAJOR;
    if (block) {
        for (auto& relay : relays)
            if (!relay.blocked)
                relay.blocked = !Firewall::blockRelay(relay);

        busy = BUSYMODE::NOT;
        return;
    }
    for (auto& relay : relays)
        if (relay.blocked)
            relay.blocked = Firewall::unblockRelay(relay);

    busy = BUSYMODE::NOT;
}

void Core::sortAlfa(std::vector<Relay>& relays) {
    busy = BUSYMODE::MINOR;
    std::sort(relays.begin(), relays.end(), [](const Relay& a, const Relay& b) {
        return a.name[0] < b.name[0];
    });
    busy = BUSYMODE::NOT;

}

void Core::sortCity(std::vector<Relay>& relays) {
    busy = BUSYMODE::MINOR;
    std::sort(relays.begin(), relays.end(), [](const Relay& a, const Relay& b) {
        auto extractCountry = [](const std::string& entry) {
            size_t openBracket = entry.find('(');
            size_t closeBracket = entry.find(')');
            if (openBracket != std::string::npos && closeBracket != std::string::npos && closeBracket > openBracket) {
                return entry.substr(openBracket + 1, closeBracket - openBracket - 1);
            }
            return std::string(""); // Relay with not included country in their name ex. Hong Kong  
        };
        return extractCountry(a.name) < extractCountry(b.name);
    });
    busy = BUSYMODE::NOT;
}

bool Core::updateSDRConfig() noexcept {
    char tempPath[MAX_PATH];
    if (!GetTempPathA(MAX_PATH, tempPath))
        return false;

    strcat_s(tempPath, "azurre_sdr.json");
    HRESULT res = URLDownloadToFile(NULL, url, tempPath, NULL, NULL);
    if (res != S_OK)
        return false;


    std::ifstream f(tempPath);
    sdr = json::parse(f);

    bool return_ = false;
    return_ = sdr["success"];

    if (return_)
        std::remove(tempPath);

    return return_; // "success" : true
}

void Core::refresh() noexcept {
    busy = BUSYMODE::MINOR;
    relays = {};
    std::string activeRules = Firewall::listOutboundFirewallRules();

    if (!updateSDRConfig())
        MessageBox(NULL, "Failed to update SDRConfig", "Azurre - Error", S_OK);

    if (sdr.empty())
        return;

    constexpr const char* pops = "pops";
    for (auto it = sdr[pops].begin(); it != sdr[pops].end(); ++it) {
        const std::string code = it.key();
        const std::string name = sdr[pops][code]["desc"];
        std::vector<std::string> IPs;
        for (const auto& relay : sdr[pops][code.c_str()]["relays"])
            IPs.push_back(relay["ipv4"].get<std::string>());
        
        const auto& enabled = activeRules.find(std::string("Azurre -> ").append(name));

        if (!IPs.empty())
            relays.push_back({ code, name, IPs, enabled != std::string::npos });
    }
    relays.shrink_to_fit();
    busy = BUSYMODE::NOT;
}

void Core::init() noexcept { // aka refresh
    refresh();
}