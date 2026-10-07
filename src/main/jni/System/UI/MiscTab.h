#pragma once

#include "ImGui/Call_ImGui.h"
#include "../ImGui/imgui_settings.h"
#include "../ImGui/custom_widgets.hpp"
#include "../Fonts/Icon.h"

#include <cfloat>
#include <cstdio>

extern ImFont* F107;

namespace font {
    extern ImFont* inter_semibold;
}

namespace misc_tab
{
    struct ChangelogSelection
    {
        const char *title;
        const char *icon;
        const char *const *items;
        int count;
        ImVec4 color;
    };

    struct LayoutMetrics
    {
        float rowGap;
        float changelogHeight;
        float infoHeight;
        float priceHeight;
    };

    static const char *const kChangelogTabs[] = {"ADDED", "REMOVED", "FIXED"};
    static const char *const kChangelogIcons[] = {ICON_FA_PLUS, ICON_FA_MINUS, ICON_FA_WRENCH};
    static const char *const kAddedItems[] = {
        "Added Optimized Fps Lock, 120 Fps In Any Device Supported, Remove Lags",
        "Separated Weapon, Character, Deadbox, and Misc into real sidebar tabs.",
        "Added popup focus dimming so the active modal stands out from the rest of the UI.",
        "Added icons to the Info popup and PHP / Dollars columns to the Pricelist popup."
    };
    static const char *const kRemovedItems[] = {
        "Removed the old Misc skin toggle list from this page.",
        "Removed the old skin-subtab flow from the Misc page.",
        "Removed the oversized popup focus padding around the modal."
    };
    static const char *const kFixedItems[] = {
        "Fixed popup dimming so the sidebar, header, children, and top status containers all sit under the same backdrop.",
        "Fixed alternating row styling for the popup tables.",
        "Fixed the popup focus cutout so it hugs the window correctly."
    };

    inline void ContentGap(float height = 6.0f)
    {
        ImGui::Dummy(ImVec2(0.0f, height));
    }

    inline bool DrawChangelogTab(const char *id, const char *icon, const char *label, bool active, const ImVec2 &size)
    {
        const ImVec2 p = ImGui::GetCursorScreenPos();
        ImGui::InvisibleButton(id, size);
        const bool clicked = ImGui::IsItemClicked();
        const bool hovered = ImGui::IsItemHovered();
        const bool held = ImGui::IsItemActive();

        ImDrawList *drawList = ImGui::GetWindowDrawList();
        ImU32 bg = IM_COL32(14, 14, 14, active ? 232 : 205);
        if (!active && hovered) bg = IM_COL32(22, 22, 22, 220);
        if (!active && held) bg = IM_COL32(31, 31, 31, 230);
        if (active && hovered) bg = IM_COL32(17, 17, 17, 245);
        if (active && held) bg = IM_COL32(5, 5, 5, 250);

        const ImU32 border = active
            ? ImGui::GetColorU32(ImVec4(c::accent.x, c::accent.y, c::accent.z, 0.82f))
            : ImGui::GetColorU32(ImVec4(c::accent.x, c::accent.y, c::accent.z, 0.28f));
        const ImVec2 max(p.x + size.x, p.y + size.y);
        drawList->AddRectFilled(p, max, bg, 4.0f);
        drawList->AddRect(p, max, border, 4.0f, 0, active ? 1.6f : 1.0f);

        ImFont *iconFont = F107 ? F107 : ImGui::GetFont();
        ImFont *labelFont = font::inter_semibold ? font::inter_semibold : ImGui::GetFont();
        const float iconSize = labelFont->FontSize * 0.82f;
        const float labelSize = labelFont->FontSize * 0.74f;
        const float gap = 7.0f;
        const ImVec2 iconTextSize = iconFont->CalcTextSizeA(iconSize, FLT_MAX, 0.0f, icon);
        const ImVec2 labelTextSize = labelFont->CalcTextSizeA(labelSize, FLT_MAX, 0.0f, label);
        const float contentW = iconTextSize.x + gap + labelTextSize.x;
        const float startX = p.x + (size.x - contentW) * 0.5f;
        const float centerY = p.y + size.y * 0.5f;

        drawList->AddText(
            iconFont,
            iconSize,
            ImVec2(startX, centerY - iconTextSize.y * 0.5f),
            active ? ImGui::GetColorU32(c::accent) : ImGui::GetColorU32(c::text::text_active),
            icon
        );
        drawList->AddText(
            labelFont,
            labelSize,
            ImVec2(startX + iconTextSize.x + gap, centerY - labelTextSize.y * 0.5f),
            ImGui::GetColorU32(c::text::text_active),
            label
        );
        return clicked;
    }

    inline void DrawSectionItems(const char *const *items, int count)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, c::text::text_active);
        for (int i = 0; i < count; ++i)
        {
            ImGui::Bullet();
            ImGui::SameLine(0.0f, 8.0f);
            ImGui::TextWrapped("%s", items[i]);
            ContentGap(2.0f);
        }
        ImGui::PopStyleColor();
    }

    inline void DrawInfoSummary()
    {
        auto drawLine = [](const char *label, const char *value, const ImVec4 &valueColor) {
            ImGui::TextColored(ImVec4(0.70f, 0.72f, 0.78f, 0.78f), "%s", label);
            ImGui::SameLine(112.0f);
            ImGui::TextColored(valueColor, "%s", value);
            ContentGap(3.0f);
        };

        drawLine("DEVELOPER", "ETHNIR NOIR", ImVec4(0.96f, 0.97f, 0.99f, 0.96f));
        drawLine("GAME", "CALL OF DUTY: MOBILE", ImVec4(0.96f, 0.97f, 0.99f, 0.96f));
        drawLine("ARCHITECTURE", "ARM64-V8A", ImVec4(0.96f, 0.97f, 0.99f, 0.96f));
        drawLine("ACCESS", "NON-ROOT", ImVec4(0.60f, 0.95f, 0.74f, 0.96f));
    }

    inline void DrawPriceSummary(const char *tableId)
    {
        ImGui::PushStyleColor(ImGuiCol_TableRowBg, ImVec4(0.055f, 0.055f, 0.055f, 0.42f));
        ImGui::PushStyleColor(ImGuiCol_TableRowBgAlt, ImVec4(0.086f, 0.086f, 0.086f, 0.56f));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.0f, 1.0f, 1.0f, 0.06f));
        if (ImGui::BeginTable(tableId, 3, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_NoSavedSettings))
        {
            ImGui::TableSetupColumn("Duration", ImGuiTableColumnFlags_WidthStretch, 0.54f);
            ImGui::TableSetupColumn("PHP", ImGuiTableColumnFlags_WidthFixed, 78.0f);
            ImGui::TableSetupColumn("USD", ImGuiTableColumnFlags_WidthFixed, 58.0f);
            ImGui::TableHeadersRow();

            auto drawRow = [](const char *duration, const char *php, const char *usd) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(ImVec4(0.93f, 0.95f, 0.98f, 0.95f), "%s", duration);
                ImGui::TableSetColumnIndex(1);
                ImGui::TextColored(ImVec4(0.557f, 0.525f, 0.965f, 0.96f), "%s", php);
                ImGui::TableSetColumnIndex(2);
                ImGui::TextColored(ImVec4(0.557f, 0.525f, 0.965f, 0.96f), "%s", usd);
            };

            drawRow("7 DAYS", "100PHP", "$2");
            drawRow("15 DAYS", "150PHP", "$3");
            drawRow("30 DAYS", "200PHP", "$4");
            drawRow("LIFETIME", "300PHP", "$5");
            ImGui::EndTable();
        }
        ImGui::PopStyleColor(3);
    }

    inline ChangelogSelection GetActiveChangelogSelection(int activeTab)
    {
        ChangelogSelection selection{
            kChangelogTabs[2],
            kChangelogIcons[2],
            kFixedItems,
            IM_ARRAYSIZE(kFixedItems),
            ImVec4(0.96f, 0.84f, 0.38f, 0.98f)
        };

        if (activeTab == 0)
        {
            selection.title = kChangelogTabs[0];
            selection.icon = kChangelogIcons[0];
            selection.items = kAddedItems;
            selection.count = IM_ARRAYSIZE(kAddedItems);
            selection.color = ImVec4(0.60f, 0.95f, 0.74f, 0.96f);
        }
        else if (activeTab == 1)
        {
            selection.title = kChangelogTabs[1];
            selection.icon = kChangelogIcons[1];
            selection.items = kRemovedItems;
            selection.count = IM_ARRAYSIZE(kRemovedItems);
            selection.color = ImVec4(1.00f, 0.68f, 0.72f, 0.96f);
        }

        return selection;
    }

    inline LayoutMetrics CalculateLayout(float contentWidth, int activeTab)
    {
        const float rowGap = 5.0f;
        const float childCapHeight = 34.0f * c::scale;
        const float childPad = c::child::padding * c::scale;
        const float innerWidth = ImMax(120.0f, contentWidth - childPad * 2.0f);
        const float bodyLineHeight = ImGui::GetTextLineHeight();
        const float bodyLineHeightSpaced = ImGui::GetTextLineHeightWithSpacing();
        const float titleRowHeight = ImMax(bodyLineHeight, ImGui::GetFontSize() * 0.96f);

        const ChangelogSelection selection = GetActiveChangelogSelection(activeTab);
        const float bulletWrapWidth = ImMax(80.0f, innerWidth - 22.0f);
        float changelogBulletHeight = 0.0f;
        for (int i = 0; i < selection.count; ++i)
        {
            const ImVec2 wrapped = ImGui::CalcTextSize(selection.items[i], nullptr, false, bulletWrapWidth);
            changelogBulletHeight += ImMax(bodyLineHeightSpaced, wrapped.y) + 14.0f;
        }

        LayoutMetrics metrics{};
        metrics.rowGap = rowGap;
        metrics.changelogHeight =
            childCapHeight +
            childPad * 2.0f +
            40.0f +
            18.0f +
            titleRowHeight +
            16.0f +
            changelogBulletHeight +
            30.0f;
        metrics.infoHeight =
            childCapHeight +
            childPad * 2.0f +
            (bodyLineHeightSpaced + 4.0f) * 4.0f +
            18.0f;
        metrics.priceHeight =
            childCapHeight +
            childPad * 2.0f +
            bodyLineHeightSpaced * 6.0f +
            18.0f;
        return metrics;
    }
}
