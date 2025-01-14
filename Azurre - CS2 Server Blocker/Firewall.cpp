#include "Firewall.h"
#include <sstream>

#include <comdef.h>
#include <netfw.h>
#include <iostream>

// Messy code, to refractor!

std::wstring string2WString(const std::string& str) {
    std::wstring wstr;
    size_t size;
    wstr.resize(str.length());
    mbstowcs_s(&size, &wstr[0], wstr.size() + 1, str.c_str(), str.size());
    return wstr;
}

std::string BSTR2STR(BSTR bstr) {
    if (!bstr)
        return "";

    _bstr_t wrapped_bstr(bstr);
    return std::string(wrapped_bstr);
}

bool Firewall::unblockRelay(Relay& relay) {
    HRESULT hr;
    INetFwPolicy2* pNetFwPolicy2 = nullptr;
    INetFwRules* pFwRules = nullptr;
    IEnumVARIANT* pEnum = nullptr;

    // Initialize COM.
    hr = CoInitializeEx(0, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) {
        std::cerr << "Failed to initialize COM: " << std::hex << hr << std::endl;
        return true;
    }

    // Create an instance of the INetFwPolicy2 interface.
    hr = CoCreateInstance(__uuidof(NetFwPolicy2), nullptr, CLSCTX_INPROC_SERVER, __uuidof(INetFwPolicy2), (void**)&pNetFwPolicy2);
    if (FAILED(hr)) {
        std::cerr << "Failed to create INetFwPolicy2 instance: " << std::hex << hr << std::endl;
        CoUninitialize();
        return true;
    }

    // Retrieve the rules collection.
    hr = pNetFwPolicy2->get_Rules(&pFwRules);
    if (FAILED(hr)) {
        std::cerr << "Failed to get firewall rules: " << std::hex << hr << std::endl;
        pNetFwPolicy2->Release();
        CoUninitialize();
        return true;
    }

    // Retrieve the enumerator for the rules.
    IUnknown* pUnknown = nullptr;
    hr = pFwRules->get__NewEnum(&pUnknown);
    if (SUCCEEDED(hr) && pUnknown != nullptr) {
        hr = pUnknown->QueryInterface(__uuidof(IEnumVARIANT), (void**)&pEnum);
        pUnknown->Release();
    }

    if (pEnum == nullptr) {
        std::cerr << "Failed to enumerate rules." << std::endl;
        pFwRules->Release();
        pNetFwPolicy2->Release();
        CoUninitialize();
        return true;
    }

    // Enumerate the rules.
    VARIANT varRule;
    VariantInit(&varRule);

    while (pEnum->Next(1, &varRule, nullptr) == S_OK) {
        INetFwRule* pFwRule = nullptr;
        if (varRule.vt == VT_DISPATCH) {
            hr = varRule.pdispVal->QueryInterface(__uuidof(INetFwRule), (void**)&pFwRule);
            if (SUCCEEDED(hr)) {
                BSTR name;
                pFwRule->get_Name(&name);

                auto rName = string2WString(std::string("Azurre -> ") + relay.name);
                const auto wRelayName = rName.c_str();

                if (name && !wcscmp(wRelayName, name)) {
                    std::wcout << L"Deleting rule: " << name << std::endl;
                    pFwRules->Remove(name);

                    SysFreeString(name);
                    pFwRule->Release();
                    VariantClear(&varRule);
                    pEnum->Release();
                    pFwRules->Release();
                    pNetFwPolicy2->Release();
                    CoUninitialize();
                    return false;
                }

                pFwRule->Release();
            }
        }
        VariantClear(&varRule);
    }

    // Cleanup.
    pEnum->Release();
    pFwRules->Release();
    pNetFwPolicy2->Release();
    CoUninitialize();
    return true;
}

bool Firewall::blockRelay(Relay& relay) {
    HRESULT hr;
    INetFwPolicy2* pNetFwPolicy2 = nullptr;
    INetFwRules* pFwRules = nullptr;
    INetFwRule* pFwRule = nullptr;

    // Initialize COM.
    hr = CoInitializeEx(0, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) {
        std::cerr << "Failed to initialize COM: " << std::hex << hr << std::endl;
        return true;
    }

    // Create an instance of the INetFwPolicy2 interface.
    hr = CoCreateInstance(__uuidof(NetFwPolicy2), nullptr, CLSCTX_INPROC_SERVER, __uuidof(INetFwPolicy2), (void**)&pNetFwPolicy2);
    if (FAILED(hr)) {
        std::cerr << "Failed to create INetFwPolicy2 instance: " << std::hex << hr << std::endl;
        CoUninitialize();
        return true;
    }

    // Retrieve the rules collection.
    hr = pNetFwPolicy2->get_Rules(&pFwRules);
    if (FAILED(hr)) {
        std::cerr << "Failed to get firewall rules: " << std::hex << hr << std::endl;
        pNetFwPolicy2->Release();
        CoUninitialize();
        return true;
    }

    // Create a new firewall rule.
    hr = CoCreateInstance(__uuidof(NetFwRule), nullptr, CLSCTX_INPROC_SERVER, __uuidof(INetFwRule), (void**)&pFwRule);
    if (FAILED(hr)) {
        std::cerr << "Failed to create INetFwRule instance: " << std::hex << hr << std::endl;
        pFwRules->Release();
        pNetFwPolicy2->Release();
        CoUninitialize();
        return true;
    }

    // Configure the rule.
    pFwRule->put_Name((BSTR)string2WString(std::string("Azurre -> ").append(relay.name)).c_str());
    pFwRule->put_Description((BSTR)L"Blocked by Azurre CS2 Server Blocker.");
    pFwRule->put_Protocol(NET_FW_IP_PROTOCOL_ANY);  // Apply to all protocols
    pFwRule->put_Direction(NET_FW_RULE_DIR_OUT);   // Outbound direction
    pFwRule->put_Action(NET_FW_ACTION_BLOCK);      // Block action
    pFwRule->put_Enabled(VARIANT_TRUE);            // Enable the rule
    pFwRule->put_Profiles(NET_FW_PROFILE2_ALL);    // Apply to all profiles

    // Specify the remote address to block.
    std::string ips = "";
    for (auto& IP : relay.IPv4s)
        ips += IP + ",";
    
    std::wstring wstr;
    size_t size;
    wstr.resize(ips.length());
    mbstowcs_s(&size, &wstr[0], wstr.size() + 1, ips.c_str(), ips.size());

    pFwRule->put_RemoteAddresses((BSTR)wstr.c_str());

    // Add the rule to the rules collection.
    hr = pFwRules->Add(pFwRule);
    if (FAILED(hr)) {
        std::cerr << "Failed to add the rule to the firewall: " << std::hex << hr << std::endl;
        // Cleanup.
        pFwRule->Release();
        pFwRules->Release();
        pNetFwPolicy2->Release();
        CoUninitialize();
        return true;
    }
    return false;
}

std::string Firewall::listOutboundFirewallRules() noexcept {
    HRESULT hr;
    INetFwPolicy2* pNetFwPolicy2 = nullptr;
    INetFwRules* pFwRules = nullptr;
    IEnumVARIANT* pEnum = nullptr;

    std::stringstream ss;

    // Initialize COM.
    hr = CoInitializeEx(0, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) {
        std::cerr << "Failed to initialize COM: " << std::hex << hr << std::endl;
        return ss.str();
    }

    // Create an instance of the INetFwPolicy2 interface.
    hr = CoCreateInstance(__uuidof(NetFwPolicy2), nullptr, CLSCTX_INPROC_SERVER, __uuidof(INetFwPolicy2), (void**)&pNetFwPolicy2);
    if (FAILED(hr)) {
        std::cerr << "Failed to create INetFwPolicy2 instance: " << std::hex << hr << std::endl;
        CoUninitialize();
        return ss.str();
    }

    // Retrieve the rules collection.
    hr = pNetFwPolicy2->get_Rules(&pFwRules);
    if (FAILED(hr)) {
        std::cerr << "Failed to get firewall rules: " << std::hex << hr << std::endl;
        pNetFwPolicy2->Release();
        CoUninitialize();
        return ss.str();
    }

    // Retrieve the enumerator for the rules.
    IUnknown* pUnknown = nullptr;
    hr = pFwRules->get__NewEnum(&pUnknown);
    if (SUCCEEDED(hr) && pUnknown != nullptr) {
        hr = pUnknown->QueryInterface(__uuidof(IEnumVARIANT), (void**)&pEnum);
        if (FAILED(hr))
            std::cerr << "Failed to query IEnumVARIANT: " << std::hex << hr << std::endl;

        pUnknown->Release(); // Release the IUnknown interface after use.
    } else
        std::cerr << "Failed to get enumerator: " << std::hex << hr << std::endl;

    // Enumerate the rules.
    if (pEnum != nullptr) {
        VARIANT varRule;
        VariantInit(&varRule);

        while (pEnum->Next(1, &varRule, nullptr) == S_OK) {
            INetFwRule* pFwRule = nullptr;
            if (varRule.vt == VT_DISPATCH) {
                hr = varRule.pdispVal->QueryInterface(__uuidof(INetFwRule), (void**)&pFwRule);
                if (SUCCEEDED(hr)) {
                    BSTR ruleName;
                    NET_FW_RULE_DIRECTION fwDirection;

                    pFwRule->get_Name(&ruleName);
                    pFwRule->get_Direction(&fwDirection);

                    if (fwDirection == NET_FW_RULE_DIR_OUT)
                        ss << BSTR2STR(ruleName) << std::endl;
                    
                    SysFreeString(ruleName);
                    pFwRule->Release();
                }
            }
            VariantClear(&varRule);
        }
        pEnum->Release();
    }

    // Cleanup.
    if (pFwRules) pFwRules->Release();
    if (pNetFwPolicy2) pNetFwPolicy2->Release();
    CoUninitialize();
    return ss.str();
}

