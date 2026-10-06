#pragma once

#include "Call_ImGui.h"
#include "imgui_settings.h"
#include "../System/Core/SaveConfig.h"
#include "../System/UI/TextureLoader.h"
#include "../System/UI/Logo.h"
#include "../System/UI/SkinTab.h"
#include "../Fonts/Icon.h"
#include "../IMAGE/bg.h"

#include <cmath>
#include <cctype>
#include <cstring>
#include <cstdio>
#include <ctime>
#include <string>

extern ImFont* F50;
extern ImFont* F107;
extern float menu[4];
extern float SnowBsize;
extern float speedHackMultiplier;
extern float jumpHeightMultiplier;
extern float SlideRange;

namespace font {
    extern ImFont* inter_semibold;
}

namespace runtime_preview_menu {

struct Texture {
    GLuint id = 0;
};

struct StateRefs {
    bool &dark;
    float &tabAlpha;
    float &tabAdd;
    int &page;
    int &activeTab;
    bool &windowCollapsed;
    bool &isMenuVisible;
    double &collapseBarLastActiveTime;
    float &collapseBarOpacityAnim;
    float &collapseBarPressAnim;
};

static Texture g_menuBackground;

static bool g_infoPopupRequested = false;
static bool g_priceListPopupRequested = false;
static bool g_popupFocusVisible = false;
static ImVec2 g_popupFocusMin(0.0f, 0.0f);
static ImVec2 g_popupFocusMax(0.0f, 0.0f);

static int g_activeChangelogTab = 0;
static int g_espBoxType = 1;
static int g_espLinePosition = 1;
static int g_espHealthPosition = 1;
static int g_espCrosshairStyle = 0;
static int g_espStyle = 0;

inline void EnsureTexture(Texture &texture, const unsigned char *bytes, int size) {
    if (texture.id != 0) {
        return;
    }
    texture.id = LoadTextureFromMemory(bytes, size);
}

inline void EnsureTexturesLoaded() {
    EnsureTexture(g_menuBackground, bg_data, (int)sizeof(bg_data));
}

inline std::string ToUpperAscii(std::string text) {
    for (char &ch : text) {
        ch = (char)std::toupper((unsigned char)ch);
    }
    return text;
}

inline std::string BuildExpiryLabel() {
    const std::string countdown = getExpiryCountdown();
    if (!countdown.empty()) {
        return ToUpperAscii("EXPIRY " + countdown);
    }
    if (EXP == "No expiry") {
        return "NO EXPIRY";
    }
    if (!EXP.empty() && EXP != " ") {
        return ToUpperAscii(EXP);
    }
    return "NO EXPIRY";
}

inline std::string BuildFpsLabel() {
    char buffer[32] = {};
    std::snprintf(buffer, sizeof(buffer), "FPS %d", (int)std::lround(ImGui::GetIO().Framerate));
    return buffer;
}

inline void ResetPopupFocusWindow() {
    g_popupFocusVisible = false;
    g_popupFocusMin = ImVec2(0.0f, 0.0f);
    g_popupFocusMax = ImVec2(0.0f, 0.0f);
}

inline void RegisterPopupFocusWindow() {
    const ImVec2 min = ImGui::GetWindowPos();
    const ImVec2 windowSize = ImGui::GetWindowSize();
    const ImVec2 max(min.x + windowSize.x, min.y + windowSize.y);
    if (!g_popupFocusVisible) {
        g_popupFocusVisible = true;
        g_popupFocusMin = min;
        g_popupFocusMax = max;
        return;
    }

    g_popupFocusMin.x = ImMin(g_popupFocusMin.x, min.x);
    g_popupFocusMin.y = ImMin(g_popupFocusMin.y, min.y);
    g_popupFocusMax.x = ImMax(g_popupFocusMax.x, max.x);
    g_popupFocusMax.y = ImMax(g_popupFocusMax.y, max.y);
}

inline void DrawPopupBackdropFocusLayer(ImDrawList *drawList) {
    if (drawList == nullptr || !g_popupFocusVisible) {
        return;
    }

    drawList->PushClipRectFullScreen();
    const ImVec2 viewportMax = ImGui::GetIO().DisplaySize;
    const ImVec2 holeMin(
        ImClamp(g_popupFocusMin.x, 0.0f, viewportMax.x),
        ImClamp(g_popupFocusMin.y, 0.0f, viewportMax.y)
    );
    const ImVec2 holeMax(
        ImClamp(g_popupFocusMax.x, 0.0f, viewportMax.x),
        ImClamp(g_popupFocusMax.y, 0.0f, viewportMax.y)
    );
    const ImU32 dimColor = IM_COL32(0, 0, 0, 172);

    if (holeMin.y > 0.0f) drawList->AddRectFilled(ImVec2(0.0f, 0.0f), ImVec2(viewportMax.x, holeMin.y), dimColor);
    if (holeMin.x > 0.0f) drawList->AddRectFilled(ImVec2(0.0f, holeMin.y), ImVec2(holeMin.x, holeMax.y), dimColor);
    if (holeMax.x < viewportMax.x) drawList->AddRectFilled(ImVec2(holeMax.x, holeMin.y), ImVec2(viewportMax.x, holeMax.y), dimColor);
    if (holeMax.y < viewportMax.y) drawList->AddRectFilled(ImVec2(0.0f, holeMax.y), viewportMax, dimColor);

    drawList->AddRect(
        holeMin,
        holeMax,
        ImGui::GetColorU32(ImVec4(c::accent.x, c::accent.y, c::accent.z, 0.18f)),
        10.0f,
        0,
        1.0f
    );
    drawList->PopClipRect();
}

#include "runtime_preview_helpers.h"
#include "../System/UI/MiscTab.h"
#include "../System/UI/SettingsTab.h"

inline void RenderComboRow(const char *label, const char *const items[], int itemCount, int *value) {
    if (label == nullptr || items == nullptr || value == nullptr || itemCount <= 0) {
        return;
    }

    *value = ImClamp(*value, 0, itemCount - 1);
    custom::Combo(label, value, items, itemCount);
}

inline bool DrawRuntimeEspColorRow(const char *label, float *color) {
    if (label == nullptr || color == nullptr) {
        return false;
    }

    auto normalize = [](float v) {
        return (v <= 1.0f) ? ImClamp(v, 0.0f, 1.0f) : ImClamp(v / 255.0f, 0.0f, 1.0f);
    };

    float col[4] = {
        normalize(color[0]),
        normalize(color[1]),
        normalize(color[2]),
        normalize(color[3])
    };

    const bool changed = custom::ColorEdit4(
        label,
        col,
        ImGuiColorEditFlags_NoInputs |
        ImGuiColorEditFlags_NoAlpha |
        ImGuiColorEditFlags_PickerHueWheel
    );

    if (changed) {
        color[0] = col[0] * 255.0f;
        color[1] = col[1] * 255.0f;
        color[2] = col[2] * 255.0f;
        color[3] = 255.0f;
    }

    return changed;
}

inline void SyncEspOptionStateFromConfig(int boxTypeCount, int linePositionCount, int healthPositionCount, int espStyleCount) {
    g_espBoxType = ImClamp((int)Config.ESPMenu.BoxType, 0, ImMax(0, boxTypeCount - 1));
    g_espLinePosition = ImClamp((int)Config.ESPMenu.Target, 0, ImMax(0, linePositionCount - 1));
    g_espHealthPosition = ImClamp((int)Config.ESPMenu.HealthPosition, 0, ImMax(0, healthPositionCount - 1));
    g_espStyle = ImClamp((int)Config.ESPMenu.EspStyle, 0, ImMax(0, espStyleCount - 1));
}

inline void CopyLinkedEspColors(float *lineColor, float *boxColor, float *nameColor, float *healthColor, float *distanceColor, float *skeletonColor) {
    if (lineColor == nullptr) {
        return;
    }

    if (boxColor) std::memcpy(boxColor, lineColor, sizeof(float) * 4);
    if (nameColor) std::memcpy(nameColor, lineColor, sizeof(float) * 4);
    if (healthColor) std::memcpy(healthColor, lineColor, sizeof(float) * 4);
    if (distanceColor) std::memcpy(distanceColor, lineColor, sizeof(float) * 4);
    if (skeletonColor) std::memcpy(skeletonColor, lineColor, sizeof(float) * 4);
}

inline void ApplyEspOptionStateToConfig() {
    Config.ESPMenu.BoxType = (EspBoxType)g_espBoxType;
    Config.ESPMenu.Target = (LineTarget)g_espLinePosition;
    Config.ESPMenu.HealthPosition = (EspHealthPosition)g_espHealthPosition;
    Config.ESPMenu.EspStyle = (EspStyleTarget)g_espStyle;
}

inline void RenderEspTab(float childWidth, float childHeight) {
    custom::BeginGroup();
    {
        const ChildFrame left = BeginContentChild("ESP##RUNTIME_ESP", ImVec2(childWidth, childHeight));
        custom::Checkbox("ESP Line", &Config.ESPMenu.isPlayerLine);
        custom::Checkbox("ESP Box", &Config.ESPMenu.Box);
        custom::Checkbox("ESP Skeleton", &Config.ESPMenu.Skeleton);
        custom::Checkbox("ESP Health", &Config.ESPMenu.Health);
        custom::Checkbox("ESP Name", &Config.ESPMenu.Name);
        custom::Checkbox("ESP Distance", &Config.ESPMenu.Distance);
        custom::Checkbox("ESP Count", &Config.ESPMenu.Count);
        custom::Checkbox("360 Alert", &Config.ESPMenu.Alert);
        custom::Checkbox("Show AimLine", &Config.ESPMenu.Aimline);
        custom::Checkbox("Yellow Wallhack", &Config.ExtraMenu.WallHack);
        custom::Checkbox("Red Wallhack", &Config.ExtraMenu.RedWallhack);
        EndContentChild(left);
    }
    custom::EndGroup();

    ImGui::SameLine(0.0f, 10.0f);

    custom::BeginGroup();
    {
        const ChildFrame right = BeginContentChild("ESP OPTIONS##RUNTIME_ESP_OPTIONS", ImVec2(childWidth, childHeight));
        static const char *boxTypes[] = {"Fill", "Outline", "Corner", "3D"};
        static const char *linePositions[] = {"Top", "Mid", "Bottom"};
        static const char *healthPositions[] = {"Top", "Side"};
        static const char *espStyles[] = {"None", "3D Sphere", "Player Signal"};
        g_espBoxType = ImClamp((int)Config.ESPMenu.BoxType, 0, IM_ARRAYSIZE(boxTypes) - 1);
        g_espLinePosition = ImClamp((int)Config.ESPMenu.Target, 0, IM_ARRAYSIZE(linePositions) - 1);
        g_espHealthPosition = ImClamp((int)Config.ESPMenu.HealthPosition, 0, IM_ARRAYSIZE(healthPositions) - 1);
        g_espStyle = ImClamp((int)Config.ESPMenu.EspStyle, 0, IM_ARRAYSIZE(espStyles) - 1);
        RenderComboRow("Box Type", boxTypes, IM_ARRAYSIZE(boxTypes), &g_espBoxType);
        RenderComboRow("Line Position", linePositions, IM_ARRAYSIZE(linePositions), &g_espLinePosition);
        RenderComboRow("Health Position", healthPositions, IM_ARRAYSIZE(healthPositions), &g_espHealthPosition);
        RenderComboRow("ESP Style", espStyles, IM_ARRAYSIZE(espStyles), &g_espStyle);

        DrawRuntimeEspColorRow("Player ESP Color", Config.sColorsESPPLAYER.LinePLAYER);
        if (Config.sColorsESPPLAYER.BoxPLAYER) memcpy(Config.sColorsESPPLAYER.BoxPLAYER, Config.sColorsESPPLAYER.LinePLAYER, sizeof(float) * 4);
        if (Config.sColorsESPPLAYER.NamePLAYER) memcpy(Config.sColorsESPPLAYER.NamePLAYER, Config.sColorsESPPLAYER.LinePLAYER, sizeof(float) * 4);
        if (Config.sColorsESPPLAYER.HealthPLAYER) memcpy(Config.sColorsESPPLAYER.HealthPLAYER, Config.sColorsESPPLAYER.LinePLAYER, sizeof(float) * 4);
        if (Config.sColorsESPPLAYER.DistancePLAYER) memcpy(Config.sColorsESPPLAYER.DistancePLAYER, Config.sColorsESPPLAYER.LinePLAYER, sizeof(float) * 4);
        if (Config.sColorsESPPLAYER.SkeletonPLAYER) memcpy(Config.sColorsESPPLAYER.SkeletonPLAYER, Config.sColorsESPPLAYER.LinePLAYER, sizeof(float) * 4);
        DrawRuntimeEspColorRow("Bot ESP Color", Config.sColorsESPBOT.LineBOT);
        if (Config.sColorsESPBOT.BoxBOT) memcpy(Config.sColorsESPBOT.BoxBOT, Config.sColorsESPBOT.LineBOT, sizeof(float) * 4);
        if (Config.sColorsESPBOT.NameBOT) memcpy(Config.sColorsESPBOT.NameBOT, Config.sColorsESPBOT.LineBOT, sizeof(float) * 4);
        if (Config.sColorsESPBOT.HealthBOT) memcpy(Config.sColorsESPBOT.HealthBOT, Config.sColorsESPBOT.LineBOT, sizeof(float) * 4);
        if (Config.sColorsESPBOT.DistanceBOT) memcpy(Config.sColorsESPBOT.DistanceBOT, Config.sColorsESPBOT.LineBOT, sizeof(float) * 4);
        if (Config.sColorsESPBOT.SkeletonBOT) memcpy(Config.sColorsESPBOT.SkeletonBOT, Config.sColorsESPBOT.LineBOT, sizeof(float) * 4);
        Config.ESPMenu.BoxType = (EspBoxType)g_espBoxType;
        Config.ESPMenu.Target = (LineTarget)g_espLinePosition;
        Config.ESPMenu.HealthPosition = (EspHealthPosition)g_espHealthPosition;
        Config.ESPMenu.EspStyle = (EspStyleTarget)g_espStyle;
        EndContentChild(right);
    }
    custom::EndGroup();
}

inline void RenderAimTab(float childWidth, float childHeight) {
    custom::BeginGroup();
    {
        const ChildFrame left = BeginContentChild("AIMBOT##RUNTIME_AIMBOT", ImVec2(childWidth, childHeight));
        custom::Checkbox("Aimbot 360", &Config.Aim.Aimbot360);
        custom::Checkbox("Bullet Track", &Config.Aim.AimSilent);
        custom::SliderFloat("Aim Assist Size", &Config.Aim.AimAssistSize, 0.0f, 100.0f, "%.0f");
        EndContentChild(left);
    }
    custom::EndGroup();

    ImGui::SameLine(0.0f, 10.0f);

    custom::BeginGroup();
    {
        const ChildFrame right = BeginContentChild("COMBAT OPTIONS##RUNTIME_COMBAT_OPTIONS", ImVec2(childWidth, childHeight));
        static const char *targets[] = {"Head", "Chest", "Body"};
        static const char *triggers[] = {"None", "Shooting", "Scoping"};
        static const char *targetBy[] = {"Distance", "FOV"};
        int target = ImClamp((int)Config.Aim.Target, 0, IM_ARRAYSIZE(targets) - 1);
        int trigger = ImClamp((int)Config.Aim.Trigger, 0, IM_ARRAYSIZE(triggers) - 1);
        int aimBy = ImClamp((int)Config.Aim.By, 0, IM_ARRAYSIZE(targetBy) - 1);
        RenderComboRow("Location", targets, IM_ARRAYSIZE(targets), &target);
        RenderComboRow("Trigger", triggers, IM_ARRAYSIZE(triggers), &trigger);
        RenderComboRow("Target By", targetBy, IM_ARRAYSIZE(targetBy), &aimBy);
        Config.Aim.Target = (EAimTarget)target;
        Config.Aim.Trigger = (EAimTrigger)trigger;
        Config.Aim.By = (EAim)aimBy;
        custom::SliderFloat("FOV Size", &Config.Aim.Cross, 0.0f, 100.0f, "%.0f");
        EndContentChild(right);
    }
    custom::EndGroup();
}

inline void RenderMemoryTab(float childWidth, float childHeight) {
    custom::BeginGroup();
    {
        const ChildFrame left = BeginContentChild("MEMORY HACKS##RUNTIME_MEMORY_LEFT", ImVec2(childWidth, childHeight));
        custom::Checkbox("Hitbox", &Config.ExtraMenu.Hit);
        custom::SliderFloat("Hitbox Size", &Config.ExtraMenu.HitboxScale, 1.0f, 15.0f, "%.1fm");
        custom::Checkbox("No Recoil", &Config.ExtraMenu.Recoil);
        custom::Checkbox("No Spread", &Config.ExtraMenu.Spread);
        custom::Checkbox("No Shake", &Config.ExtraMenu.Shake);
        custom::Checkbox("No Overheat", &Config.ExtraMenu.Rpd);
        custom::Checkbox("No Parachute", &Config.ExtraMenu.Parachute);
        custom::Checkbox("Anti Flashbang", &Config.ExtraMenu.Flash);
        custom::Checkbox("Firerate", &Config.ExtraMenu.Fire);
        custom::Checkbox("Fast Dive", &Config.ExtraMenu.Diving);
        custom::Checkbox("Fast Reload", &Config.ExtraMenu.Reload);
        custom::Checkbox("Fast Scope", &Config.ExtraMenu.Scope);
        custom::Checkbox("Quick Switch", &Config.ExtraMenu.Switch);
        custom::Checkbox("Weapon Kinetic", &Config.ExtraMenu.Kinetic);
        custom::Checkbox("No Crouch", &Config.ExtraMenu.NoCrouch);
        EndContentChild(left);
    }
    custom::EndGroup();

    ImGui::SameLine(0.0f, 10.0f);

    custom::BeginGroup();
    {
        const ChildFrame right = BeginContentChild("MISC FEATURES##RUNTIME_MEMORY_RIGHT", ImVec2(childWidth, childHeight));
        custom::SliderFloat("Snowboard Speed", &SnowBsize, 0.0f, 100.0f, "%.1f");
        custom::SliderFloat("Slide Distance", &SlideRange, 0.0f, 30.0f, "%.1f");
        custom::SliderFloat("SpeedHack", &speedHackMultiplier, 0.5f, 2.0f, "%.1fx");
        custom::SliderFloat("High Jump", &jumpHeightMultiplier, 0.5f, 5.0f, "%.2fx");
        EndContentChild(right);
    }
    custom::EndGroup();
}

inline void RenderMiscTab(float contentWidth, float contentHeight) {
    const ChildFrame misc = BeginContentChild("MISC##MISC", ImVec2(contentWidth, contentHeight));
    const misc_tab::LayoutMetrics layout = misc_tab::CalculateLayout(contentWidth, g_activeChangelogTab);

    {
        const ChildFrame changelog = BeginContentChild("CHANGELOG##MISC_CHANGELOG", ImVec2(contentWidth, layout.changelogHeight), ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

        const float changelogTabGap = 5.0f;
        const float changelogTabH = 40.0f;
        const float changelogSafeRight = ImGui::GetStyle().ScrollbarSize + 12.0f;
        const float changelogAvailW = ImMax(1.0f, ImGui::GetContentRegionAvail().x - changelogSafeRight);
        const float changelogTabW = ImMax(1.0f, ImFloor((changelogAvailW - changelogTabGap * 2.0f) / 3.0f));
        for (int i = 0; i < IM_ARRAYSIZE(misc_tab::kChangelogTabs); ++i) {
            char buttonId[48] = {};
            std::snprintf(buttonId, sizeof(buttonId), "##changelog_%d", i);
            if (i > 0) {
                ImGui::SameLine(0.0f, changelogTabGap);
            }
            if (misc_tab::DrawChangelogTab(buttonId, misc_tab::kChangelogIcons[i], misc_tab::kChangelogTabs[i], g_activeChangelogTab == i, ImVec2(changelogTabW, changelogTabH))) {
                g_activeChangelogTab = i;
            }
        }

        misc_tab::ContentGap(12.0f);

        const misc_tab::ChangelogSelection activeSection = misc_tab::GetActiveChangelogSelection(g_activeChangelogTab);

        ImFont *iconFont = F107 ? F107 : ImGui::GetFont();
        ImGui::PushFont(iconFont);
        ImGui::TextColored(activeSection.color, "%s", activeSection.icon);
        ImGui::PopFont();
        ImGui::SameLine(0.0f, 8.0f);
        ImGui::TextColored(activeSection.color, "%s", activeSection.title);
        ImGui::SameLine(0.0f, 10.0f);
        ImGui::TextColored(c::text::text, "March 10, 2026");
        misc_tab::ContentGap(8.0f);
        misc_tab::DrawSectionItems(activeSection.items, activeSection.count);
        EndContentChild(changelog);
    }

    misc_tab::ContentGap(layout.rowGap);

    {
        const ChildFrame info = BeginContentChild("INFO##MISC_INFO", ImVec2(contentWidth, layout.infoHeight), ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        misc_tab::DrawInfoSummary();
        EndContentChild(info);
    }

    misc_tab::ContentGap(layout.rowGap);

    {
        const ChildFrame price = BeginContentChild("PRICELIST##MISC_PRICELIST", ImVec2(contentWidth, layout.priceHeight), ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        misc_tab::DrawPriceSummary("##runtime_misc_inline_prices");
        EndContentChild(price);
    }

    EndContentChild(misc);
}

inline void RenderSettingsTab(float contentWidth, float contentHeight) {
    ImGuiStyle &style = ImGui::GetStyle();
    const float columnGap = ImClamp(contentWidth * 0.022f, 12.0f, 18.0f);
    const float rowGap = 6.0f;
    const float leftChildWidth = ImMax(0.0f, ImFloor((contentWidth - columnGap) * 0.5f));
    const float rightChildWidth = ImMax(0.0f, contentWidth - columnGap - leftChildWidth);
    const float topChildHeight = ImMax(0.0f, ImFloor((contentHeight - rowGap) * 0.5f));
    const float bottomChildHeight = ImMax(0.0f, contentHeight - rowGap - topChildHeight);
    const float startX = ImGui::GetCursorPosX();

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(style.ItemSpacing.x, rowGap));

    ImGui::SetCursorPosX(startX);
    custom::BeginGroup();
    {
        const ChildFrame licenseChild = BeginContentChild("LICENSE INFO##RUNTIME_LICENSE_INFO", ImVec2(leftChildWidth, topChildHeight));
        settings_tab::RenderLicenseCard();
        EndContentChild(licenseChild);

        const ChildFrame logoChild = BeginContentChild("LOGO SETTINGS##RUNTIME_LOGO_SETTINGS", ImVec2(leftChildWidth, bottomChildHeight));
        settings_tab::RenderLogoCard();
        EndContentChild(logoChild);
    }
    custom::EndGroup();

    ImGui::SameLine(0.0f, columnGap);

    custom::BeginGroup();
    {
        const ChildFrame configChild = BeginContentChild("CONFIG MANAGEMENT##RUNTIME_CONFIG_MANAGEMENT", ImVec2(rightChildWidth, topChildHeight));
        settings_tab::RenderConfigCard();
        EndContentChild(configChild);

        const ChildFrame enhancementChild = BeginContentChild("ENHANCEMENT##RUNTIME_ENHANCEMENT", ImVec2(rightChildWidth, bottomChildHeight));
        settings_tab::RenderEnhancementCard();
        EndContentChild(enhancementChild);
    }
    custom::EndGroup();

    ImGui::PopStyleVar();
}


inline bool DrawRuntimeSidebarButton(const char *id, const char *icon, const char *label, bool active, const ImVec2 &min, const ImVec2 &size) {
    if (id == nullptr || label == nullptr) {
        return false;
    }

    ImGui::SetCursorScreenPos(min);
    ImGui::InvisibleButton(id, size);
    const bool hovered = ImGui::IsItemHovered();
    const bool clicked = ImGui::IsItemClicked();

    ImDrawList *drawList = ImGui::GetWindowDrawList();
    const ImU32 bg = active
        ? main_runtime_theme::GetAccentTintU32(0.28f, 0.92f)
        : hovered ? IM_COL32(255, 255, 255, 10) : IM_COL32(0, 0, 0, 0);
    if (bg != IM_COL32(0, 0, 0, 0)) {
        drawList->AddRectFilled(min, min + size, bg, 9.0f);
    }

    if (active) {
        drawList->AddRect(min, min + size, main_runtime_theme::GetAccentU32(0.95f), 9.0f, 0, 1.0f);
        drawList->AddRectFilled(
            ImVec2(min.x, min.y + 5.0f),
            ImVec2(min.x + 3.0f, min.y + size.y - 5.0f),
            main_runtime_theme::GetAccentU32(0.95f),
            2.0f
        );
    }

    ImFont *iconFont = custom::shell::GetIconFont();
    ImFont *textFont = font::inter_semibold ? font::inter_semibold : ImGui::GetFont();
    const float iconSize = 18.0f;
    const float textSize = 14.0f;
    const ImU32 iconColor = active ? main_runtime_theme::GetAccentU32(1.0f) : IM_COL32(180, 180, 184, 210);
    const ImU32 textColor = active ? IM_COL32(235, 235, 235, 255) : IM_COL32(180, 180, 184, 235);

    if (icon != nullptr && iconFont != nullptr) {
        const ImVec2 is = iconFont->CalcTextSizeA(iconSize, FLT_MAX, 0.0f, icon);
        drawList->AddText(iconFont, iconSize,
            ImVec2(min.x + 15.0f, min.y + (size.y - is.y) * 0.5f), iconColor, icon);
    }

    if (textFont != nullptr) {
        const ImVec2 ts = textFont->CalcTextSizeA(textSize, FLT_MAX, 0.0f, label);
        drawList->AddText(textFont, textSize,
            ImVec2(min.x + 47.0f, min.y + (size.y - ts.y) * 0.5f), textColor, label);
    }

    return clicked;
}

inline void DrawRuntimeEspPreviewPanel(const ImVec2 &min, const ImVec2 &size) {
    ImDrawList *drawList = ImGui::GetWindowDrawList();
    const ImVec2 max = min + size;
    drawList->AddRectFilled(min, max, IM_COL32(14, 14, 14, 245), 10.0f);
    drawList->AddRect(min, max, IM_COL32(45, 45, 45, 220), 10.0f, 0, 1.0f);

    ImFont *textFont = font::inter_semibold ? font::inter_semibold : ImGui::GetFont();
    const float titleSize = 15.0f;
    const char *title = "ESP PREVIEW";
    const ImVec2 titleSz = textFont->CalcTextSizeA(titleSize, FLT_MAX, 0.0f, title);
    drawList->AddText(textFont, titleSize,
        ImVec2(min.x + 14.0f, min.y + 12.0f), main_runtime_theme::GetAccentU32(1.0f), title);

    const ImVec2 previewMin(min.x + 12.0f, min.y + 42.0f);
    const ImVec2 previewMax(max.x - 12.0f, max.y - 14.0f);
    drawList->AddRectFilled(previewMin, previewMax, IM_COL32(22, 22, 22, 245), 7.0f);

    // Lightweight visual-only preview. It does not read or modify gameplay state.
    const float groundY = previewMax.y - 34.0f;
    drawList->AddRectFilled(ImVec2(previewMin.x, groundY), previewMax, IM_COL32(17, 17, 17, 255), 0.0f);
    drawList->AddLine(ImVec2(previewMin.x, groundY), ImVec2(previewMax.x, groundY), IM_COL32(45, 45, 45, 220), 1.0f);

    const ImVec2 boxCenter((previewMin.x + previewMax.x) * 0.5f, (previewMin.y + groundY) * 0.5f + 8.0f);
    const float boxW = ImMin(58.0f, (previewMax.x - previewMin.x) * 0.42f);
    const float boxH = ImMin(120.0f, (groundY - previewMin.y) * 0.55f);
    const ImVec2 boxMin(boxCenter.x - boxW * 0.5f, boxCenter.y - boxH * 0.5f);
    const ImVec2 boxMax(boxCenter.x + boxW * 0.5f, boxCenter.y + boxH * 0.5f);

    drawList->AddRect(boxMin, boxMax, IM_COL32(255, 48, 48, 255), 2.0f, 0, 2.0f);
    drawList->AddRectFilled(ImVec2(boxMin.x, boxMin.y - 9.0f), ImVec2(boxMax.x, boxMin.y - 5.0f), IM_COL32(42, 255, 75, 255), 2.0f);

    const char *enemy = "Enemy";
    const ImVec2 enemySz = textFont->CalcTextSizeA(12.0f, FLT_MAX, 0.0f, enemy);
    drawList->AddText(textFont, 12.0f,
        ImVec2(boxCenter.x - enemySz.x * 0.5f, boxMin.y - 27.0f), IM_COL32(255, 64, 64, 255), enemy);

    const char *distance = "45m";
    const ImVec2 distanceSz = textFont->CalcTextSizeA(11.0f, FLT_MAX, 0.0f, distance);
    drawList->AddText(textFont, 11.0f,
        ImVec2(boxCenter.x - distanceSz.x * 0.5f, boxMax.y + 7.0f), IM_COL32(255, 74, 74, 255), distance);
}

inline void CollapseMenu(StateRefs &state) {
    state.windowCollapsed = true;
    state.isMenuVisible = false;
    state.collapseBarLastActiveTime = ImGui::GetTime();
    state.collapseBarOpacityAnim = 1.0f;
    state.collapseBarPressAnim = 0.0f;
}

} // namespace runtime_preview_menu
