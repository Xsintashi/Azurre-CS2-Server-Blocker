#include "Core.h"
#include "Firewall.h"
#include "Lib/nlohmann/json.hpp"

using json = nlohmann::json;
json sdr{};

void Core::sortAlfa(std::vector<Relay>& relays) {
    std::sort(relays.begin(), relays.end(), [](const Relay& a, const Relay& b) {
        return a.name[0] < b.name[0];
    });
}

void Core::sortCity(std::vector<Relay>& relays) {
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

void Core::init() noexcept { // aka refresh
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
        for (const auto& relay : sdr[pops][code.c_str()]["relays"]) {
            IPs.push_back(relay["ipv4"].get<std::string>());
        }

        const auto& enabled = activeRules.find(std::string("Azurre -> ").append(name));

        if (!IPs.empty())
            relays.push_back({code, name, IPs, enabled != std::string::npos});
    }
    relays.shrink_to_fit();
}