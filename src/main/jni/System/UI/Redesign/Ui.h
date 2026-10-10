#pragma once
// ============================================================================
// ETHNIR NOIR V3 — redesigned UI shell
//
// Replaces the pizza wheel with a floating "hub" launcher (2x3 icon grid,
// draggable, position remembered in ui_layout.ini) and restyles the content
// window with a left icon rail instead of text pills. All tab contents are
// reused unchanged; only the shell around them is new.
// ============================================================================

#include "imgui.h"
#include "imgui_internal.h"
#include "../../Includes/Logger.h"

namespace redesign {

struct Theme
{
    static ImU32 Accent()
    {
        float hr, hg, hb;
        ImGui::ColorConvertHSVtoRGB(ImClamp(main_runtime_theme::g_menuHue, 0.0f, 1.0f), 0.50f, 0.97f, hr, hg, hb);
        return IM_COL32((int)(hr * 255.0f), (int)(hg * 255.0f), (int)(hb * 255.0f), 255);
    }
    static ImU32 AccentDim()
    {
        float hr, hg, hb;
        ImGui::ColorConvertHSVtoRGB(ImClamp(main_runtime_theme::g_menuHue, 0.0f, 1.0f), 0.50f, 0.80f, hr, hg, hb);
        return IM_COL32((int)(hr * 160.0f), (int)(hg * 160.0f), (int)(hb * 160.0f), 170);
    }
    static ImU32 Bg()        { return IM_COL32(10, 10, 12, 248); }   // card body
    static ImU32 BgSoft()    { return IM_COL32(18, 18, 22, 235); }   // tile rest
    static ImU32 BgHover()   { return IM_COL32(28, 28, 34, 250); }   // tile hover
    static ImU32 Border()    { return IM_COL32(44, 44, 54, 200); }
    static ImU32 Text()      { return IM_COL32(232, 232, 238, 255); }
    static ImU32 TextMut()   { return IM_COL32(146, 146, 156, 255); }
};

// Shared draw helper: glass rounded card with a thin top accent line.
inline void DrawCard(ImDrawList *dl, const ImVec2 &mn, const ImVec2 &mx, float rounding, float alphaMul = 1.0f)
{
    dl->AddRectFilled(mn, mx, IM_COL32(0, 0, 0, (int)(90 * alphaMul)), rounding);
    dl->AddRectFilled(mn, mx, IM_COL32((Theme::Bg() >> IM_COL32_R_SHIFT) & 0xFF,
                                       (Theme::Bg() >> IM_COL32_G_SHIFT) & 0xFF,
                                       (Theme::Bg() >> IM_COL32_B_SHIFT) & 0xFF,
                                       (int)(248 * alphaMul)), rounding);
    dl->AddRect(mn, mx, Theme::Border(), rounding, 0, 1.2f);
    const ImU32 acc = Theme::Accent();
    dl->AddRectFilled(mn, ImVec2(mx.x, mn.y + 2.5f), acc, rounding, ImDrawFlags_RoundCornersTop);
}

// ---------------------------------------------------------------------------
// HUB LAUNCHER — replaces the pizza wheel.
// Returns 1..6 when the user taps a tile, 0 otherwise.
// ---------------------------------------------------------------------------
inline int RenderHub(const ImVec2 &defaultCenter)
{
    constexpr float kPi = 3.14159265358979323846f;
    const ImVec2 display = ImGui::GetIO().DisplaySize;

    constexpr float hubW = 430.0f, hubH = 330.0f;

    static ImVec2 hubPos(0, 0);
    static ImVec2 hubPosApplied(-99999, -99999);
    static bool hubInit = false;
    static bool dragging = false;
    if (!hubInit)
    {
        hubInit = true;
        const ui_layout::State &layout = ui_layout::Get();
        hubPos = layout.hasWheel
            ? ImVec2(layout.wheelX, layout.wheelY)
            : ImVec2(defaultCenter.x - hubW * 0.5f, defaultCenter.y - hubH * 0.5f);
    }

    // Keep on-screen.
    hubPos.x = ImClamp(hubPos.x, 4.0f, ImMax(4.0f, display.x - hubW - 4.0f));
    hubPos.y = ImClamp(hubPos.y, 4.0f, ImMax(4.0f, display.y - hubH - 4.0f));
    if (hubPos.x != hubPosApplied.x || hubPos.y != hubPosApplied.y)
    {
        ImGui::SetNextWindowPos(hubPos, ImGuiCond_Always);
        hubPosApplied = hubPos;
    }
    ImGui::SetNextWindowSize(ImVec2(hubW, hubH), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.0f);

    int picked = 0;
    const ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground |
        ImGuiWindowFlags_NoBringToFrontOnFocus;

    struct Tile { const char *icon; const char *label; };
    static const Tile tiles[6] = {
        { ICON_FA_PALETTE,    "VISUAL"   },
        { ICON_FA_CROSSHAIRS, "COMBAT"   },
        { ICON_FA_MICROCHIP,  "MEMORY"   },
        { ICON_FA_SHIELD_ALT, "SKINS"    },
        { ICON_FA_EYE,        "MISC"     },
        { ICON_FA_COG,        "SETTINGS" },
    };

    if (ImGui::Begin("##redesign_hub", nullptr, flags))
    {
        ImDrawList *dl = ImGui::GetWindowDrawList();
        const ImVec2 wp = ImGui::GetWindowPos();

        ImGui::SetCursorPos(ImVec2(0, 0));
        ImGui::InvisibleButton("##hub_hitbox", ImVec2(hubW, hubH));
        const bool held = ImGui::IsItemActive();
        if (held && !dragging && ImGui::IsMouseDragging(ImGuiMouseButton_Left, 8.0f))
            dragging = true;

        if (dragging && ImGui::IsMouseDown(ImGuiMouseButton_Left))
        {
            hubPos = ImVec2(hubPos.x + ImGui::GetIO().MouseDelta.x, hubPos.y + ImGui::GetIO().MouseDelta.y);
            ImGui::SetWindowPos(hubPos, ImGuiCond_Always);
            hubPosApplied = hubPos;
        }
        if (!ImGui::IsMouseDown(ImGuiMouseButton_Left) && dragging)
        {
            dragging = false;
            ui_layout::RememberWheel(hubPos.x, hubPos.y);
        }

        const float t = (float)ImGui::GetTime();
        const float pulse = 0.5f + 0.5f * std::sin(t * 2.2f);

        DrawCard(dl, wp, wp + ImVec2(hubW, hubH), 18.0f);

        // Header
        ImFont *titleFont = custom::shell::GetTitleFont();
        const char *l1 = "ETHNIR NOIR";
        const char *l2 = " CONTAINER V3";
        const ImVec2 s1 = titleFont ? titleFont->CalcTextSizeA(20.0f, FLT_MAX, 0.0f, l1) : ImGui::CalcTextSize(l1);
        const ImVec2 s2 = titleFont ? titleFont->CalcTextSizeA(20.0f, FLT_MAX, 0.0f, l2) : ImGui::CalcTextSize(l2);
        const float titleX = wp.x + (hubW - s1.x - s2.x) * 0.5f;
        if (titleFont) {
            dl->AddText(titleFont, 20.0f, ImVec2(titleX, wp.y + 18.0f), Theme::Text(), l1);
            dl->AddText(titleFont, 20.0f, ImVec2(titleX + s1.x, wp.y + 18.0f), Theme::Accent(), l2);
        }
        dl->AddRectFilled(ImVec2(wp.x + 150.0f, wp.y + 46.0f), ImVec2(wp.x + hubW - 150.0f, wp.y + 47.5f),
                          IM_COL32(60, 60, 72, 120), 1.0f);

        // 2 x 3 tile grid
        const float gridTop = wp.y + 62.0f;
        const float gridBottom = wp.y + hubH - 14.0f;
        const float pad = 14.0f;
        const float tileGap = 12.0f;
        const float tileW = (hubW - pad * 2.0f - tileGap * 2.0f) / 3.0f;
        const float tileH = (gridBottom - gridTop - tileGap) / 2.0f;

        for (int i = 0; i < 6; ++i)
        {
            const int col = i % 3, row = i / 3;
            const ImVec2 tmin(wp.x + pad + col * (tileW + tileGap), gridTop + row * (tileH + tileGap));
            const ImVec2 tmax(tmin.x + tileW, tmin.y + tileH);

            char id[32];
            snprintf(id, sizeof(id), "##hub_tile_%d", i);
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0, 0, 0, 0));
            ImGui::SetCursorScreenPos(tmin);
            const bool clicked = ImGui::Button(id, ImVec2(tileW, tileH));
            ImGui::PopStyleColor(5);

            const bool hovered = ImGui::IsItemHovered();
            const bool heldTile = ImGui::IsItemActive();

            // Only treat as press-drag when the pointer leaves the tile while held;
            // a plain tap never becomes a drag.
            if (heldTile) ImGui::SetItemDefaultFocus();

            const float breathe = hovered ? (0.5f + 0.5f * pulse) : 0.0f;

            ImU32 bgCol = IM_COL32(18, 18, 22, 228);
            ImU32 fgCol = Theme::TextMut();
            ImU32 borderCol = Theme::Border();
            if (hovered || heldTile)
            {
                const ImU32 acc = Theme::Accent();
                bgCol = IM_COL32(30, 30, 38, 250);
                fgCol = IM_COL32(255, 255, 255, 255);
                borderCol = acc;
                dl->AddRectFilled(tmin, tmax, IM_COL32((acc >> IM_COL32_R_SHIFT) & 0xFF,
                                                       (acc >> IM_COL32_G_SHIFT) & 0xFF,
                                                       (acc >> IM_COL32_B_SHIFT) & 0xFF,
                                                       (int)(38 + 34 * breathe)), 14.0f);
            }
            dl->AddRectFilled(tmin, tmax, bgCol, 14.0f);
            dl->AddRect(tmin, tmax, borderCol, 14.0f, 0, heldTile ? 1.8f : 1.1f);

            ImFont *iconFont = custom::shell::GetIconFont();
            const ImVec2 isz = custom::shell::MeasureText(iconFont, 26.0f, tiles[i].icon);
            const ImVec2 csz = ImGui::CalcTextSize(tiles[i].label);
            const ImVec2 ic(tmin.x + (tileW - isz.x) * 0.5f, tmin.y + tileH * 0.30f - isz.y * 0.5f);
            const ImVec2 cc(tmin.x + (tileW - csz.x) * 0.5f, tmin.y + tileH * 0.66f);
            if (iconFont) dl->AddText(iconFont, 26.0f, ic, (hovered || heldTile) ? Theme::Accent() : fgCol, tiles[i].icon);
            dl->AddText(cc, (hovered || heldTile) ? Theme::Text() : fgCol, tiles[i].label);

            if (clicked && !dragging)
                picked = i + 1;
        }

        // Footer hint
        const char *hint = "Tap a tile to open - drag the card to move";
        const ImVec2 hsz = ImGui::CalcTextSize(hint);
        dl->AddText(ImVec2(wp.x + (hubW - hsz.x) * 0.5f, wp.y + hubH - 10.0f - hsz.y), IM_COL32(110, 110, 122, 190), hint);
    }
    ImGui::End();
    return picked;
}

// ---------------------------------------------------------------------------
// LOGIN CARD — restyled auth window, same behaviour/inputs as before.
// The actual input handling (key buffer, PASTE/LOG IN, keyboard) stays in
// Main.cpp; this draws the frame + title + helper text and reports layout.
// ---------------------------------------------------------------------------
inline void DrawLoginChrome(ImDrawList *dl, const ImVec2 &pos, const ImVec2 &size)
{
    const float rounding = 16.0f;

    // Hovering glow behind everything
    const ImU32 acc = Theme::Accent();
    const float glowR = 90.0f + 8.0f * std::sin((float)ImGui::GetTime() * 1.7f);
    dl->AddRectFilledMultiColor(ImVec2(pos.x + 40.0f, pos.y + size.y * 0.55f - glowR),
                                ImVec2(pos.x + size.x - 40.0f, pos.y + size.y * 0.55f + glowR),
                                IM_COL32((acc >> IM_COL32_R_SHIFT) & 0xFF, (acc >> IM_COL32_G_SHIFT) & 0xFF,
                                         (acc >> IM_COL32_B_SHIFT) & 0xFF, 26),
                                IM_COL32((acc >> IM_COL32_R_SHIFT) & 0xFF, (acc >> IM_COL32_G_SHIFT) & 0xFF,
                                         (acc >> IM_COL32_B_SHIFT) & 0xFF, 4),
                                IM_COL32(0, 0, 0, 0), IM_COL32(0, 0, 0, 0));

    // Card
    dl->AddRectFilled(pos, pos + size, IM_COL32(10, 10, 13, 235), rounding);
    dl->AddRect(pos, pos + size, Theme::Border(), rounding, 0, 1.4f);
    dl->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + 3.0f), acc, rounding, ImDrawFlags_RoundCornersTop);

    // Shield logo circle
    const ImVec2 logoC(pos.x + size.x * 0.5f, pos.y + 64.0f);
    dl->AddCircleFilled(logoC, 34.0f, IM_COL32(22, 22, 28, 255), 40);
    dl->AddCircle(logoC, 34.0f, acc, 40, 2.0f);
    ImFont *iconFont = custom::shell::GetIconFont();
    if (iconFont)
    {
        const ImVec2 isz = custom::shell::MeasureText(iconFont, 24.0f, ICON_FA_SHIELD_ALT);
        dl->AddText(iconFont, 24.0f, ImVec2(logoC.x - isz.x * 0.5f, logoC.y - isz.y * 0.5f), acc, ICON_FA_SHIELD_ALT);
    }

    // Title
    ImFont *titleFont = custom::shell::GetTitleFont();
    const char *t1 = "ETHNIR NOIR";
    const char *t2 = " LOGIN";
    const ImVec2 ts1 = titleFont ? titleFont->CalcTextSizeA(26.0f, FLT_MAX, 0.0f, t1) : ImGui::CalcTextSize(t1);
    const ImVec2 ts2 = titleFont ? titleFont->CalcTextSizeA(26.0f, FLT_MAX, 0.0f, t2) : ImGui::CalcTextSize(t2);
    const float tX = pos.x + (size.x - ts1.x - ts2.x) * 0.5f;
    if (titleFont)
    {
        dl->AddText(titleFont, 26.0f, ImVec2(tX, pos.y + 112.0f), Theme::Text(), t1);
        dl->AddText(titleFont, 26.0f, ImVec2(tX + ts1.x, pos.y + 112.0f), acc, t2);
    }

    const char *sub = "SIGN IN WITH YOUR LICENSE KEY";
    const ImVec2 ssz = ImGui::CalcTextSize(sub);
    dl->AddText(ImVec2(pos.x + (size.x - ssz.x) * 0.5f, pos.y + 148.0f), Theme::TextMut(), sub);
}

} // namespace redesign
