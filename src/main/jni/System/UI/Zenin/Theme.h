#pragma once
// ============================================================================
// ZENIN THEME — pixel-faithful port of the referenced zenin menu.
//
// Look: deep charcoal panels, soft red accent (#FF4C4C family), large rounded
// section cards with a centered title and an accent underline, row separators,
// square accent checkboxes on the right, rounded green-slider style sliders,
// rounded grey buttons, and a bottom dock bar (moon toggle centered, category
// icons in the middle, folder/settings on the right).
//
// Everything is drawn directly with the draw list so it matches the reference
// exactly instead of inheriting ImGui's default widget look.
// ============================================================================

#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "ImGui/Call_ImGui.h"
#include "ImGui/imgui_settings.h"
#include "Fonts/Iconcpp.h"
#include "System/Core/UiLayout.h"

#include <cmath>

namespace zenin
{

// ---------------------------------------------------------------------------
// Palette
// ---------------------------------------------------------------------------
namespace T
{
    inline ImU32 WindowBg      = IM_COL32(15, 15, 16, 255);   // outer card
    inline ImU32 SectionBg     = IM_COL32(24, 24, 26, 255);   // big section cards
    inline ImU32 RowHover      = IM_COL32(32, 32, 35, 255);
    inline ImU32 Text          = IM_COL32(228, 228, 232, 255);
    inline ImU32 TextMut       = IM_COL32(120, 120, 126, 255);
    inline ImU32 Accent        = IM_COL32(255, 90, 92, 255);  // zenin red
    inline ImU32 AccentSoft    = IM_COL32(255, 90, 92, 60);
    inline ImU32 Separator     = IM_COL32(46, 46, 50, 160);
    inline ImU32 ButtonBg      = IM_COL32(36, 36, 40, 255);
    inline ImU32 ButtonHover   = IM_COL32(46, 46, 52, 255);
    inline ImU32 DockBg        = IM_COL32(24, 24, 27, 255);
    inline ImU32 IconDim       = IM_COL32(150, 150, 158, 255);

    inline float RWindow       = 12.0f;   // reference bg::rounding
    inline float RSection      = 8.0f;    // reference child::rounding
    inline float RRow          = 8.0f;    // reference element::rounding

    // ---------------------------------------------------------------------
    // The shell used to hard-code the "zenin red" palette while the widgets
    // read the runtime accent, so half the menu never followed the theme.
    // RefreshPalette() re-derives every shell colour from the single palette
    // in ImGui/imgui_settings.h, and is called once per frame right after the
    // style is applied. Accent now comes from the runtime hue -- whose default
    // is the reference indigo (101,87,255).
    // ---------------------------------------------------------------------
    inline void RefreshPalette()
    {
        const ImU32 accent = main_runtime_theme::GetAccentU32(1.0f);

        WindowBg    = ImGui::GetColorU32(c::bg::background);
        SectionBg   = ImGui::GetColorU32(c::child::cap);
        RowHover    = ImGui::GetColorU32(c::elements::background_hovered);
        Text        = ImGui::GetColorU32(c::text::text_active);
        TextMut     = ImGui::GetColorU32(c::text::text);
        Accent      = accent;
        AccentSoft  = main_runtime_theme::GetAccentTintU32(0.94f, 0.22f);
        Separator   = ImGui::GetColorU32(c::separator);
        ButtonBg    = ImGui::GetColorU32(c::button::background);
        ButtonHover = ImGui::GetColorU32(c::button::background_hovered);
        DockBg      = ImGui::GetColorU32(c::child::cap);

        IconDim     = ImGui::GetColorU32(c::text::text);
    }
} // namespace T

// ---------------------------------------------------------------------------
// Strict helper set
// ---------------------------------------------------------------------------
inline float Clampf(float v, float a, float b)
{
    return (v < a) ? a : ((v > b) ? b : v);
}

inline float Maxf(float a, float b)
{
    return (a > b) ? a : b;
}

// Legacy-overload guard: callers MUST use the strict helpers above. Any
// accidental use of the ambiguous 2-arg/global forms fails at compile time.
struct Disallow { Disallow(...) {} };
inline void ImClamp(...) {}
inline void ImFloor(...) {}
inline void ImMin(...) {}
inline void ImMax2(...) {}

inline ImU32 AccentAlpha(int a)
{
    return IM_COL32(255, 90, 92, a);
}

// ---------------------------------------------------------------------------
// Section card: big rounded panel with a centered title + accent underline
// ---------------------------------------------------------------------------
inline void BeginSection(const ImVec2 &min, const ImVec2 &max, const char *title, ImFont *font)
{
    ImDrawList *dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(min, max, T::SectionBg, T::RSection);
    if (title != nullptr && font != nullptr)
    {
        const ImVec2 ts = font->CalcTextSizeA(20.0f, FLT_MAX, 0.0f, title);
        const float cx = (min.x + max.x) * 0.5f;
        dl->AddText(font, 20.0f, ImVec2(cx - ts.x * 0.5f, min.y + 12.0f), T::Text, title);
        // thin accent underline under the title
        const float uy = min.y + 12.0f + ts.y + 6.0f;
        // guard the underline width against tiny cards
        const float uw = Maxf(1.0f, (max.x - min.x) * 0.35f);
        dl->AddLine(ImVec2(cx - uw * 0.5f, uy), ImVec2(cx + uw * 0.5f, uy), T::Accent, 1.5f);
    }
    ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(0, 0, 0, 0));
}

inline void EndSection()
{
    ImGui::PopStyleColor();
}

// ---------------------------------------------------------------------------
// Checkbox row: label left, square accent check on the right (58px tall)
// ---------------------------------------------------------------------------
inline bool RowCheckbox(const char *label, bool *v, ImFont *font)
{
    if (label == nullptr || v == nullptr)
        return false;

    ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(0, 0, 0, 0));
    ImGui::BeginChild(label, ImVec2(ImGui::GetContentRegionAvail().x, 52.0f), false, ImGuiWindowFlags_NoScrollbar);
    bool changed = false;

    const ImVec2 rmin = ImGui::GetWindowPos();
    const ImVec2 rmax = ImVec2(rmin.x + ImGui::GetWindowWidth(), rmin.y + 52.0f);
    ImDrawList *dl = ImGui::GetWindowDrawList();

    // hover wash
    if (ImGui::IsWindowHovered())
        dl->AddRectFilled(rmin, rmax, T::RowHover, T::RRow);
    // row separator
    dl->AddLine(ImVec2(rmin.x + 10.0f, rmax.y), ImVec2(rmax.x - 10.0f, rmax.y), T::Separator, 1.0f);

    if (font != nullptr)
        dl->AddText(font, 20.0f, ImVec2(rmin.x + 10.0f, rmin.y + (52.0f - 20.0f) * 0.5f), T::Text, label);

    // square box (right aligned)
    const float box = 34.0f;
    const ImVec2 bmin(rmax.x - box - 12.0f, rmin.y + (52.0f - box) * 0.5f);
    const ImVec2 bmax(bmin.x + box, bmin.y + box);

    ImGui::SetCursorScreenPos(bmin);
    ImGui::PushID(label);
    if (ImGui::InvisibleButton("##box", ImVec2(box, box)))
    {
        *v = !*v;
        changed = true;
    }
    ImGui::PopID();

    const bool hovered2 = ImGui::IsItemHovered();
    if (hovered2)
        dl->AddRectFilled(bmin, bmax, T::RowHover, T::RRow);

    dl->AddRectFilled(bmin, bmax, *v ? T::Accent : T::ButtonBg, 9.0f);
    if (*v)
    {
        // check mark
            const ImVec2 c1(bmin.x + box * 0.28f, bmin.y + box * 0.52f);
        const ImVec2 c2(bmin.x + box * 0.45f, bmin.y + box * 0.70f);
        const ImVec2 c3(bmin.x + box * 0.72f, bmin.y + box * 0.32f);
        dl->AddLine(c1, c2, IM_COL32(255, 255, 255, 255), 3.5f);
        dl->AddLine(c2, c3, IM_COL32(255, 255, 255, 255), 3.5f);
    }
    else
    {
        dl->AddRect(bmin, bmax, IM_COL32(70, 70, 78, 255), 9.0f, 0, 1.5f);
    }

    ImGui::EndChild();
    ImGui::PopStyleColor();
    return changed;
}

// ---------------------------------------------------------------------------
// Slider row: label left, value right, slim accent bar with round grab
// ---------------------------------------------------------------------------
inline bool RowSlider(const char *label, float *v, float vmin, float vmax, const char *fmt, ImFont *font)
{
    if (label == nullptr || v == nullptr || fmt == nullptr)
        return false;

    ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(0, 0, 0, 0));
    ImGui::BeginChild(label, ImVec2(ImGui::GetContentRegionAvail().x, 62.0f), false, ImGuiWindowFlags_NoScrollbar);
    bool changed = false;

    const ImVec2 rmin = ImGui::GetWindowPos();
    const ImVec2 rmax = ImVec2(rmin.x + ImGui::GetWindowWidth(), rmin.y + 62.0f);
    ImDrawList *dl = ImGui::GetWindowDrawList();

    if (ImGui::IsWindowHovered())
        dl->AddRectFilled(rmin, rmax, T::RowHover, T::RRow);
    dl->AddLine(ImVec2(rmin.x + 10.0f, rmax.y), ImVec2(rmax.x - 10.0f, rmax.y), T::Separator, 1.0f);

    if (font != nullptr)
        dl->AddText(font, 20.0f, ImVec2(rmin.x + 10.0f, rmin.y + 8.0f), T::Text, label);

    // value on the right
    char valbuf[32];
    std::snprintf(valbuf, sizeof(valbuf), fmt, *v);
    if (font != nullptr)
    {
        const ImVec2 vs = font->CalcTextSizeA(20.0f, FLT_MAX, 0.0f, valbuf);
        dl->AddText(font, 20.0f, ImVec2(rmax.x - 12.0f - vs.x, rmin.y + 8.0f), T::TextMut, valbuf);
    }

    // slider track
    const float trackY = rmin.y + 42.0f;
    const float trackMinX = rmin.x + 12.0f;
    const float trackMaxX = rmax.x - 12.0f;
    const float h = 16.0f;

    // guard division against degenerate ranges
    float t = (vmax - vmin > 0.0001f) ? (Clampf(*v, vmin, vmax) - vmin) / (vmax - vmin) : 0.0f;

    const ImVec2 tmin(trackMinX, trackY - h * 0.5f);
    const ImVec2 tmax(trackMaxX, trackY + h * 0.5f);
    // fill
    dl->AddRectFilled(tmin, ImVec2(trackMinX + (trackMaxX - trackMinX) * t, tmax.y), T::ButtonBg, h * 0.5f);
    dl->AddRectFilled(tmax - ImVec2((trackMaxX - trackMinX) * (1.0f - t), 0.0f), tmax, T::ButtonBg, h * 0.5f);
    // grab
    const float gx = trackMinX + (trackMaxX - trackMinX) * t;
    dl->AddCircleFilled(ImVec2(gx, trackY), h * 0.5f + 2.0f, T::Accent, 20);

    ImGui::SetCursorScreenPos(ImVec2(trackMinX, trackY - 14.0f));
    ImGui::PushID(label);
    if (ImGui::InvisibleButton("##drag", ImVec2(ImMax(1.0f, trackMaxX - trackMinX), 26.0f)))
        changed = true;
    if (ImGui::IsItemActive())
    {
        const float mx = ImGui::GetIO().MousePos.x;
        const float nt = Clampf((mx - trackMinX) / ImMax(1.0f, trackMaxX - trackMinX), 0.0f, 1.0f);
        const float nv = vmin + nt * (vmax - vmin);
        if (nv != *v)
        {
            *v = nv;
            changed = true;
        }
    }
    ImGui::PopID();

    ImGui::EndChild();
    ImGui::PopStyleColor();
    return changed;
}

// ---------------------------------------------------------------------------
// Rounded grey button ("Change World" style)
// ---------------------------------------------------------------------------
inline bool ActionButton(const char *label, const ImVec2 &size, ImFont *font)
{
    if (label == nullptr)
        return false;
    const ImVec2 p = ImGui::GetCursorScreenPos();
    const ImVec2 sz(ImMax(1.0f, size.x), ImMax(1.0f, size.y));
    ImGui::InvisibleButton(label, sz);
    const bool pressed = ImGui::IsItemClicked();
    const bool hovered = ImGui::IsItemHovered();

    ImDrawList *dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(p, p + sz, pressed ? T::ButtonHover : (hovered ? T::ButtonHover : T::ButtonBg), 10.0f);
    if (font != nullptr)
    {
        const ImVec2 ts = font->CalcTextSizeA(20.0f, FLT_MAX, 0.0f, label);
        dl->AddText(font, 20.0f, ImVec2(p.x + (sz.x - ts.x) * 0.5f, p.y + (sz.y - ts.y) * 0.5f), T::Accent, label);
    }
    return pressed;
}

// ---------------------------------------------------------------------------
// Text button row (label left, value/hex on the right + a colour swatch)
// ---------------------------------------------------------------------------
inline void RowTextValue(const char *label, const char *value, const ImU32 swatch, ImFont *font)
{
    if (label == nullptr || value == nullptr || font == nullptr)
        return;
    const ImVec2 rmin = ImGui::GetCursorScreenPos();
    const float w = ImMax(1.0f, ImGui::GetContentRegionAvail().x);
    const ImVec2 rmax(rmin.x + w, rmin.y + 52.0f);
    ImDrawList *dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(rmin, rmax, ImGui::IsWindowHovered() ? T::RowHover : IM_COL32(0, 0, 0, 0) & IM_COL32(255,255,255,0), T::RRow);
    dl->AddLine(ImVec2(rmin.x + 10.0f, rmax.y), ImVec2(rmax.x - 10.0f, rmax.y), T::Separator, 1.0f);
    dl->AddText(font, 20.0f, ImVec2(rmin.x + 10.0f, rmin.y + (52.0f - 20.0f) * 0.5f), T::Text, label);
    const ImVec2 vs = font->CalcTextSizeA(20.0f, FLT_MAX, 0.0f, value);
    dl->AddText(font, 20.0f, ImVec2(rmax.x - 60.0f - vs.x - 12.0f, rmin.y + (52.0f - 20.0f) * 0.5f), T::Accent, value);
    // swatch
    dl->AddRectFilled(ImVec2(rmax.x - 60.0f, rmin.y + 14.0f), ImVec2(rmax.x - 20.0f, rmin.y + 38.0f), swatch, 8.0f);
    dl->AddRect(ImVec2(rmax.x - 60.0f, rmin.y + 14.0f), ImVec2(rmax.x - 20.0f, rmin.y + 38.0f), IM_COL32(70, 70, 78, 255), 8.0f, 0, 1.2f);
}

// ---------------------------------------------------------------------------
// Bottom dock bar: moon toggle (left) + category icons (centre) + right icons
// ---------------------------------------------------------------------------
struct DockResult
{
    int  pickedTab;   // 1..6 (0 = none)
    bool toggleDark;  // moon pressed
};

inline DockResult DockBar(const ImVec2 &min, const ImVec2 &max, const char *windowTitle,
                          const char *const icons[], int iconCount, int activeTab,
                          bool darkOn, ImFont *font)
{
    DockResult res{0, false};

    ImDrawList *dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(min, max, T::DockBg, 12.0f);

    const float barH = max.y - min.y;
    const float barW = max.x - min.x;

    // moon toggle (left)
    {
        const ImVec2 bsize(110.0f, 44.0f);
        const ImVec2 bmin(min.x + 14.0f, min.y + (barH - bsize.y) * 0.5f);
        ImGui::SetCursorScreenPos(bmin);
        char id[64];
        std::snprintf(id, sizeof(id), "##dock_moon_%s", windowTitle != nullptr ? windowTitle : "?");
        ImGui::InvisibleButton(id, bsize);
        if (ImGui::IsItemClicked())
            res.toggleDark = true;
        const bool hov = ImGui::IsItemHovered();
        dl->AddRectFilled(bmin, bmin + bsize, hov ? T::ButtonHover : T::ButtonBg, 10.0f);
        const ImU32 moonCol = darkOn ? T::Accent : T::IconDim;
        if (font != nullptr)
        {
            const ImVec2 is = font->CalcTextSizeA(22.0f, FLT_MAX, 0.0f, ICON_FA_MOON);
            dl->AddText(font, 22.0f, ImVec2(bmin.x + (bsize.x - is.x) * 0.5f, bmin.y + (bsize.y - is.y) * 0.5f), moonCol, ICON_FA_MOON);
        }
    }

    // category icons (centre group)
    if (icons != nullptr && iconCount > 0)
    {
        const float iconBtn = 46.0f;
        const float gap = 10.0f;
        const float totalW = iconBtn * iconCount + gap * (iconCount - 1);
        const float startX = min.x + (barW - totalW) * 0.5f;

        for (int i = 0; i < iconCount; ++i)
        {
            const ImVec2 bmin(startX + i * (iconBtn + gap), min.y + (barH - iconBtn) * 0.5f);
            ImGui::SetCursorScreenPos(bmin);
            char id[64];
            std::snprintf(id, sizeof(id), "##dock_icon_%s_%d", windowTitle != nullptr ? windowTitle : "?", i);
            ImGui::PushID(id);
            ImGui::InvisibleButton("##ib", ImVec2(iconBtn, iconBtn));
            const bool pressed = ImGui::IsItemClicked();
            const bool hov = ImGui::IsItemHovered();
            ImGui::PopID();

            const bool active = (activeTab == i + 1);
            if (active)
            {
                dl->AddRectFilled(bmin, bmin + ImVec2(iconBtn, iconBtn), T::AccentSoft, 10.0f);
                dl->AddRectFilled(ImVec2(bmin.x + 4.0f, bmin.y + iconBtn - 4.0f),
                                  ImVec2(bmin.x + iconBtn - 4.0f, bmin.y + iconBtn - 1.0f),
                                  T::Accent, 2.0f);
            }
            else if (hov)
            {
                dl->AddRectFilled(bmin, bmin + ImVec2(iconBtn, iconBtn), T::RowHover, 10.0f);
            }
            if (font != nullptr)
            {
                const ImVec2 is = font->CalcTextSizeA(20.0f, FLT_MAX, 0.0f, icons[i]);
                const ImU32 col = pressed ? T::Accent : (active ? T::Accent : T::IconDim);
                dl->AddText(font, 20.0f, ImVec2(bmin.x + (iconBtn - is.x) * 0.5f, bmin.y + (iconBtn - is.y) * 0.5f), col, icons[i]);
            }
            if (pressed)
                res.pickedTab = i + 1;
        }
    }

    // right side: folder + cog
    {
        const float iconBtn = 44.0f;
        const float gap = 10.0f;
        const ImVec2 fs(max.x - (iconBtn * 2.0f + gap) - 14.0f, min.y + (barH - iconBtn) * 0.5f);

        ImGui::SetCursorScreenPos(fs);
        char id[64];
        std::snprintf(id, sizeof(id), "##dock_folder_%s", windowTitle != nullptr ? windowTitle : "?");
        ImGui::PushID(id);
        ImGui::InvisibleButton("##fold", ImVec2(iconBtn, iconBtn));
        const bool foldPressed = ImGui::IsItemClicked();
        const bool foldHov = ImGui::IsItemHovered();
        ImGui::PopID();
        dl->AddRectFilled(fs, fs + ImVec2(iconBtn, iconBtn), foldHov ? T::ButtonHover : T::ButtonBg, 10.0f);
        if (font != nullptr)
        {
            const ImVec2 is = font->CalcTextSizeA(20.0f, FLT_MAX, 0.0f, ICON_FA_FOLDER);
            dl->AddText(font, 20.0f, ImVec2(fs.x + (iconBtn - is.x) * 0.5f, fs.y + (iconBtn - is.y) * 0.5f), T::IconDim, ICON_FA_FOLDER);
        }

        const ImVec2 cs(fs.x + iconBtn + gap, fs.y);
        ImGui::SetCursorScreenPos(cs);
        std::snprintf(id, sizeof(id), "##dock_cog_%s", windowTitle != nullptr ? windowTitle : "?");
        ImGui::PushID(id);
        ImGui::InvisibleButton("##cog", ImVec2(iconBtn, iconBtn));
        const bool cogPressed = ImGui::IsItemClicked();
        const bool cogHov = ImGui::IsItemHovered();
        ImGui::PopID();
        dl->AddRectFilled(cs, cs + ImVec2(iconBtn, iconBtn), cogHov ? T::ButtonHover : T::ButtonBg, 10.0f);
        if (font != nullptr)
        {
            const ImVec2 is = font->CalcTextSizeA(20.0f, FLT_MAX, 0.0f, ICON_FA_COG);
            dl->AddText(font, 20.0f, ImVec2(cs.x + (iconBtn - is.x) * 0.5f, cs.y + (iconBtn - is.y) * 0.5f), T::IconDim, ICON_FA_COG);
        }

        if (foldPressed)
            res.pickedTab = 4; // SKINS tab (folder = library/skins)
        if (cogPressed)
            res.pickedTab = 6; // SETTINGS
    }

    return res;
}

static int g_zeninHubSel = 0; // cached hub selection (0..6) set at pick time

// ---------------------------------------------------------------------------
// HUB LAUNCHER — the imgui-ref container boiled down into a draggable card:
// red-topbar strip ("ZENIN | ETHNIR NOIR V3"), thin divider, a 150px sidebar
// nav (icon + label rows, animated accent selection like the Lumin sidebar)
// and a container area with the six tab cards. Drag position persists in
// ui_layout.ini (RememberWheel slot, same as the old wheel).
// ---------------------------------------------------------------------------
inline int RenderZeninHub(const ImVec2 &defaultCenter)
{
    const ImVec2 display = ImGui::GetIO().DisplaySize;

    constexpr float hubW = 470.0f, hubH = 300.0f;
    constexpr float sidebarW = 150.0f;
    constexpr float topbarH = 52.0f;

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
    hubPos.x = Clampf(hubPos.x, 4.0f, Maxf(4.0f, display.x - hubW - 4.0f));
    hubPos.y = Clampf(hubPos.y, 4.0f, Maxf(4.0f, display.y - hubH - 4.0f));
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

    if (ImGui::Begin("##zenin_hub", nullptr, flags))
    {
        ImDrawList *dl = ImGui::GetWindowDrawList();
        const ImVec2 wp = ImGui::GetWindowPos();

        ImFont *titleFont = custom::shell::GetTitleFont();
        ImFont *textFont  = custom::shell::GetTextFont();
        ImFont *iconFont  = custom::shell::GetIconFont();

        // ---- window shell (zenin card) ----
        dl->AddRectFilled(wp, wp + ImVec2(hubW, hubH), T::WindowBg, T::RWindow);
        dl->AddRect(wp, wp + ImVec2(hubW, hubH), IM_COL32(38, 38, 42, 230), T::RWindow, 0, 1.3f);
        dl->AddRectFilled(wp, ImVec2(wp.x + hubW, wp.y + 3.0f), T::Accent, T::RWindow, ImDrawFlags_RoundCornersTop);

        // ---- topbar strip: split-color title + thin red underline ----
        {
            const char *l1 = "ZENIN";
            const char *l2 = " | ETHNIR NOIR V3";
            const float ts = 19.0f;
            const ImVec2 s1 = titleFont ? titleFont->CalcTextSizeA(ts, FLT_MAX, 0.0f, l1) : ImGui::CalcTextSize(l1);
            if (titleFont)
            {
                dl->AddText(titleFont, ts, ImVec2(wp.x + 20.0f, wp.y + 14.0f), IM_COL32(235, 235, 236, 255), l1);
                dl->AddText(titleFont, ts, ImVec2(wp.x + 20.0f + s1.x + 2.0f, wp.y + 14.0f), T::Accent, l2);
            }
            dl->AddLine(ImVec2(wp.x + 14.0f, wp.y + topbarH), ImVec2(wp.x + hubW - 14.0f, wp.y + topbarH),
                        IM_COL32(100, 30, 32, 220), 1.6f);
        }

        // ---- sidebar nav (bg panel + rows) ----
        static const char *const navIcons[6] = {
            ICON_FA_PALETTE, ICON_FA_CROSSHAIRS, ICON_FA_MICROCHIP,
            ICON_FA_SHIELD_ALT, ICON_FA_EYE, ICON_FA_COG
        };
        static const char *const navLabels[6] = {
            "VISUAL", "COMBAT", "MEMORY", "SKINS", "MISC", "SETTINGS"
        };

        const float sideTop = topbarH + 10.0f;
        const float sideBot = hubH - 34.0f;
        const float sideMinX = wp.x + 12.0f;
        const float sideMaxX = sideMinX + sidebarW;
        dl->AddRectFilled(ImVec2(sideMinX, sideTop), ImVec2(sideMaxX, sideBot), T::SectionBg, 12.0f);

        const float rowH = 33.0f;
        const float rowGap = 3.0f;
        const float rowsTop = sideTop + 8.0f;
        static ImVec4 selRect(0, 0, 0, 0);       // animated accent selection (Lumin easing)
        static bool   selValid = false;
        float hoverRect[4] = {0, 0, 0, 0};

        for (int i = 0; i < 6; ++i)
        {
            const float rx = sideMinX + 6.0f;
            const float ry = rowsTop + i * (rowH + rowGap);
            const ImVec2 rmin(rx, ry);
            const ImVec2 rmax(rx + sidebarW - 12.0f, ry + rowH);
            const bool active = (g_zeninHubSel == i + 1);

            char rowId[48];
            std::snprintf(rowId, sizeof(rowId), "##hubnav_%d", i);
            ImGui::SetCursorScreenPos(rmin);
            ImGui::PushID(rowId);
            ImGui::InvisibleButton("##navrow", ImVec2(rmax.x - rmin.x, rowH));
            const bool pressed = ImGui::IsItemClicked() && !dragging;
            const bool hov = ImGui::IsItemHovered();
            ImGui::PopID();

            if (hov && !dragging) { hoverRect[0]=rmin.x; hoverRect[1]=rmin.y; hoverRect[2]=rmax.x; hoverRect[3]=rmax.y; }
            if (hov && ImGui::IsMouseDragging(ImGuiMouseButton_Left, 10.0f))
                dragging = true;

            if (active && !selValid)
            {
                selRect = ImVec4(rmin.x, rmin.y, rmax.x, rmax.y);
                selValid = true;
            }
            else if (active)
            {
                const float e = Clampf(ImGui::GetIO().DeltaTime * 14.0f, 0.0f, 1.0f);
                selRect.x += (rmin.x - selRect.x) * e;
                selRect.y += (rmin.y - selRect.y) * e;
                selRect.z += (rmax.x - selRect.z) * e;
                selRect.w += (rmax.y - selRect.w) * e;
            }

            if (active)
            {
                dl->AddRectFilled(ImVec2(selRect.x, selRect.y), ImVec2(selRect.z, selRect.w), T::AccentSoft, 9.0f);
                // left accent bar inside the row
                dl->AddRectFilled(ImVec2(selRect.x + 2.0f, selRect.y + 4.0f),
                                  ImVec2(selRect.x + 4.0f, selRect.w - 4.0f), T::Accent, 1.0f);
            }
            else if (hov)
            {
                dl->AddRectFilled(rmin, rmax, IM_COL32(32, 32, 36, 160), 9.0f);
            }

            // icon
            if (iconFont)
            {
                const ImVec2 isz = iconFont->CalcTextSizeA(15.0f, FLT_MAX, 0.0f, navIcons[i]);
                dl->AddText(iconFont, 15.0f,
                            ImVec2(rmin.x + 10.0f, rmin.y + (rowH - isz.y) * 0.5f),
                            active ? T::Accent : T::IconDim, navIcons[i]);
            }
            // label
            if (textFont)
            {
                dl->AddText(textFont, 12.5f, ImVec2(rmin.x + 32.0f, rmin.y + (rowH - 15.0f) * 0.5f),
                            active ? T::Text : T::TextMut, navLabels[i]);
            }

            if (pressed)
                picked = i + 1;
        }

        // ---- container area (right of sidebar) ----
        const float contMinX = sideMaxX + 10.0f;
        const float contMinY = sideTop;
        const float contMaxX = wp.x + hubW - 12.0f;
        const float contMaxY = sideBot;
        dl->AddRectFilled(ImVec2(contMinX, contMinY), ImVec2(contMaxX, contMaxY), IM_COL32(10, 10, 12, 255), 12.0f);

        if (textFont)
        {
            const char *cap = "TAP A TAB TO OPEN";
            dl->AddText(textFont, 11.0f, ImVec2(contMinX + 12.0f, contMinY + 8.0f), T::TextMut, cap);
        }

        // 2x3 tab card grid
        {
            const float gx = contMinX + 10.0f, gy = contMinY + 26.0f;
            const float cw = (contMaxX - contMinX - 30.0f) * 0.5f;
            const float ch = (contMaxY - gy - 18.0f) * 0.5f - 4.0f;
            const float gapX = 10.0f, gapY = 8.0f;
            for (int i = 0; i < 6; ++i)
            {
                const float cx = gx + (i % 2) * (cw + gapX);
                const float cy = gy + (i / 2) * (ch + gapY);
                const ImVec2 cmin(cx, cy), cmax(cx + cw, cy + ch);
                const bool active = (g_zeninHubSel == i + 1);

                char cid[40];
                std::snprintf(cid, sizeof(cid), "##hubcard_%d", i);
                ImGui::SetCursorScreenPos(cmin);
                ImGui::PushID(cid);
                ImGui::InvisibleButton("##card", ImVec2(cw, ch));
                const bool pressed = ImGui::IsItemClicked() && !dragging;
                const bool hov = ImGui::IsItemHovered();
                ImGui::PopID();

                if (hov && ImGui::IsMouseDragging(ImGuiMouseButton_Left, 10.0f))
                    dragging = true;

                dl->AddRectFilled(cmin, cmax, active ? T::SectionBg : IM_COL32(24, 24, 26, 200), 10.0f);
                if (active) dl->AddRect(cmin, cmax, T::Accent, 10.0f, 0, 1.2f);
                else if (hov) dl->AddRect(cmin, cmax, IM_COL32(70, 70, 76, 200), 10.0f, 0, 1.0f);

                if (iconFont)
                {
                    const ImVec2 isz = iconFont->CalcTextSizeA(20.0f, FLT_MAX, 0.0f, navIcons[i]);
                    dl->AddText(iconFont, 20.0f,
                                ImVec2(cmin.x + (cw - isz.x) * 0.5f, cmin.y + 8.0f),
                                active ? T::Accent : T::IconDim, navIcons[i]);
                }
                if (textFont)
                {
                    dl->AddText(textFont, 10.5f,
                                ImVec2(cmin.x + (cw - 44.0f) * 0.5f, cmax.y - 22.0f),
                                active ? T::Text : T::TextMut, navLabels[i]);
                }
                if (pressed)
                    picked = i + 1;
            }
        }

        // ---- footer hint ----
        if (textFont)
        {
            const char *hint = "DRAG THE CARD TO MOVE";
            const ImVec2 hsz = textFont->CalcTextSizeA(11.0f, FLT_MAX, 0.0f, hint);
            dl->AddText(textFont, 11.0f, ImVec2(wp.x + (hubW - hsz.x) * 0.5f, wp.y + hubH - 24.0f), T::TextMut, hint);
        }

        // ---- drag handling (whole card, ignores interactive rows) ----
        if (!dragging && ImGui::IsWindowHovered() &&
            ImGui::IsMouseDragging(ImGuiMouseButton_Left, 10.0f) &&
            !ImGui::IsAnyItemHovered())
            dragging = true;
        if (dragging && ImGui::IsMouseDown(ImGuiMouseButton_Left))
        {
            hubPos.x += ImGui::GetIO().MouseDelta.x;
            hubPos.y += ImGui::GetIO().MouseDelta.y;
            ImGui::SetWindowPos(hubPos, ImGuiCond_Always);
            hubPosApplied = hubPos;
        }
        if (!ImGui::IsMouseDown(ImGuiMouseButton_Left) && dragging)
        {
            ui_layout::RememberWheel(hubPos.x, hubPos.y);
            dragging = false;
        }
    }
    ImGui::End();
    return picked;
}
} // namespace zenin
