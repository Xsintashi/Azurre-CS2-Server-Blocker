#pragma once

#include "imgui.h"
#include <sstream>
namespace ImGuiCustom
{
    void colorPicker(const char* name, float color[3], float* alpha = nullptr, bool* rainbow = nullptr, float* rainbowSpeed = nullptr, bool* enable = nullptr, float* thickness = nullptr, float* rounding = nullptr, bool* outline = nullptr) noexcept;

    template <typename F>
    void multiCombo(const char* name, F& flagValue, const char* items) noexcept
    {
        constexpr auto singleStringGetter = [](void* data, int idx, const char** outText) noexcept
        {
            const char* itemsSeparatedByZeros = (const char*)data;
            int itemsCount = 0;
            const char* p = itemsSeparatedByZeros;
            while (*p)
            {
                if (idx == itemsCount)
                    break;
                p += std::strlen(p) + 1;
                itemsCount++;
            }
            if (!*p)
                return false;
            if (outText)
                *outText = p;
            return true;
        };

        int count = 0;
        const char* p = items;
        while (*p)
        {
            p += std::strlen(p) + 1;
            count++;
        }

        void* data = (void*)items;


        std::ostringstream ss;
        if (flagValue == (1 << count) - 1)
            ss << "All";
        else if (!flagValue)
            ss << "None";
        else {
            for (int i = 0; i < count; i++) {
                const char* item;
                bool selected = flagValue & (1 << i);
                singleStringGetter(data, i, &item);
                if (selected) {
                    ss << item << " "; // Add ", " later ;/
                }
            }
        }

        if (ImGui::BeginCombo(name, ss.str().c_str()))
        {
            for (int i = 0; i < count; i++)
            {
                bool selected = flagValue & (1 << i);

                const char* item;
                singleStringGetter(data, i, &item);

                ImGui::PushID(i);
                ImGui::Selectable(item, &selected, ImGuiSelectableFlags_DontClosePopups);
                ImGui::PopID();

                if (selected)
                    flagValue |= (1 << i);
                else
                    flagValue &= ~(1 << i);
            }
            ImGui::EndCombo();
        }
    }
}

namespace ImGui {
    void textUnformattedCentered(const char* text) noexcept;
    void progressBarFullWidth(float fraction, float height) noexcept;
}