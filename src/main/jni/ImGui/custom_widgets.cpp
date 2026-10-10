#include "custom_widgets.hpp"
#include "imgui_settings.h"
#include "Icon.h"
extern ImFont* F50;
extern ImFont* F107;
namespace custom
{
    namespace shell
    {
        // Reference typography: Inter Medium for body text, the bold cut for
        // titles, the icon font for glyphs. Falls back progressively so a font
        // that failed to build can never leave the shell without a font.
        ImFont* GetTextFont()
        {
            if (font::inter_medium_widget)
                return font::inter_medium_widget;
            if (font::inter_semibold)
                return font::inter_semibold;
            if (F50)
                return F50;
            return ImGui::GetFont();
        }

        ImFont* GetTitleFont()
        {
            if (font::inter_medium_big)
                return font::inter_medium_big;
            if (F50)
                return F50;
            return GetTextFont();
        }

        ImFont* GetSmallFont()
        {
            if (font::inter_medium_small)
                return font::inter_medium_small;
            return GetTextFont();
        }

        ImFont* GetCardTitleFont()
        {
            if (font::inter_bold)
                return font::inter_bold;
            return GetTextFont();
        }

        ImFont* GetIconFont()
        {
            if (F107)
                return F107;
            if (font::icomoon_default)
                return font::icomoon_default;
            return GetTextFont();
        }

        ImVec2 MeasureText(ImFont* font, float size, const char* text)
        {
            if (font == nullptr || text == nullptr)
                return ImVec2(0.0f, 0.0f);
            return font->CalcTextSizeA(size, FLT_MAX, 0.0f, text);
        }

        float Normalize(float value, float minValue, float maxValue)
        {
            const float range = maxValue - minValue;
            if (range <= 0.0001f)
                return 0.0f;
            return ImClamp((value - minValue) / range, 0.0f, 1.0f);
        }

        void DrawPanelShell(ImDrawList* drawList, const ImVec2& min, const ImVec2& max, float rounding, bool highlighted)
        {
            if (drawList == nullptr)
                return;

            drawList->AddRectFilled(
                min,
                max,
                ImGui::GetColorU32(highlighted ? main_runtime_theme::GetActiveTabBackgroundColor() : main_runtime_theme::GetSidebarShellBackgroundColor()),
                rounding
            );
            drawList->AddRectFilledMultiColor(
                min,
                ImVec2(max.x, min.y + (max.y - min.y) * 0.48f),
                main_runtime_theme::GetAccentTintU32(0.22f, highlighted ? 0.12f : 0.08f),
                main_runtime_theme::GetAccentTintU32(0.16f, highlighted ? 0.07f : 0.04f),
                IM_COL32(0, 0, 0, 0),
                IM_COL32(0, 0, 0, 0)
            );
        }

        bool DrawSidebarButton(const char* id, const char* icon, bool active, const ImVec2& min, const ImVec2& size)
        {
            ImGui::SetCursorScreenPos(min);
            ImGui::InvisibleButton(id, size);
            const bool hovered = ImGui::IsItemHovered();
            const bool clicked = ImGui::IsItemClicked();

            ImDrawList* drawList = ImGui::GetWindowDrawList();
            const float slotSide = ImMax(1.0f, ImMin(size.y - 2.0f, size.x - 12.0f));
            const ImVec2 slotMin(min.x + (size.x - slotSide) * 0.5f, min.y + (size.y - slotSide) * 0.5f);
            const ImVec2 slotMax(slotMin.x + slotSide, slotMin.y + slotSide);
            if (active || hovered)
            {
                drawList->AddRectFilled(
                    slotMin,
                    slotMax,
                    active ? ImGui::GetColorU32(main_runtime_theme::GetActiveTabBackgroundColor()) : IM_COL32(31, 31, 31, 200),
                    9.0f
                );
            }

            ImFont* iconFont = GetIconFont();
            const float iconSize = 18.5f;
            const ImVec2 iconTextSize = MeasureText(iconFont, iconSize, icon);
            drawList->AddText(
                iconFont,
                iconSize,
                ImVec2(slotMin.x + (slotSide - iconTextSize.x) * 0.5f, slotMin.y + (slotSide - iconTextSize.y) * 0.5f),
                active ? main_runtime_theme::GetAccentTintU32(0.98f, 1.0f) : IM_COL32(180, 180, 184, hovered ? 255 : 220),
                icon
            );

            return clicked;
        }

        void DrawTitleCard(const ImVec2& min, const ImVec2& size, const char* title)
        {
            ImDrawList* drawList = ImGui::GetWindowDrawList();
            DrawPanelShell(drawList, min, ImVec2(min.x + size.x, min.y + size.y), 11.0f, true);

            ImFont* titleFont = GetTitleFont();
            // The title font is now the reference's 25px Inter bold cut, so use
            // its real size instead of the old x1.45 fudge that only existed to
            // grow the 30px fallback face.
            const float titleFontSize = (titleFont != nullptr) ? titleFont->FontSize : 24.0f;

            // Support "Static|Accent" split-color titles
            const char* pipe = title ? strchr(title, '|') : nullptr;
            if (pipe != nullptr) {
                // Render left part (static white)
                const ImVec2 leftSize = titleFont->CalcTextSizeA(titleFontSize, FLT_MAX, 0.0f, title, pipe);
                const float totalW = [&]() {
                    const char* after = pipe + 1;
                    const ImVec2 rightSize = titleFont->CalcTextSizeA(titleFontSize, FLT_MAX, 0.0f, after);
                    return leftSize.x + rightSize.x;
                }();
                const float startX = min.x + 14.0f;
                const float textY   = min.y + (size.y - leftSize.y) * 0.5f - 1.0f;

                // "Zenx" — static warm white
                drawList->AddText(
                    titleFont, titleFontSize,
                    ImVec2(startX, textY),
                    IM_COL32(235, 235, 235, 255),
                    title, pipe
                );

                // "Veld" — accent color with a soft text glow
                const char* accentPart = pipe + 1;
                const ImVec2 accentTextSize = titleFont->CalcTextSizeA(titleFontSize, FLT_MAX, 0.0f, accentPart);
                const float accentX = startX + leftSize.x;
                const ImVec4 acc = c::accent;
                const ImU32 accentCol = IM_COL32(
                    (int)(acc.x * 255), (int)(acc.y * 255),
                    (int)(acc.z * 255), 255
                );
                // Soft glow: two blurred shadow layers behind the accent text
                const ImU32 glowCol1 = IM_COL32(
                    (int)(acc.x * 255), (int)(acc.y * 255),
                    (int)(acc.z * 255), 55
                );
                const ImU32 glowCol2 = IM_COL32(
                    (int)(acc.x * 255), (int)(acc.y * 255),
                    (int)(acc.z * 255), 28
                );
                for (int dx = -2; dx <= 2; ++dx) {
                    for (int dy = -2; dy <= 2; ++dy) {
                        if (dx == 0 && dy == 0) continue;
                        const float dist = (float)(dx * dx + dy * dy);
                        const ImU32 c = dist <= 2 ? glowCol1 : glowCol2;
                        drawList->AddText(titleFont, titleFontSize,
                            ImVec2(accentX + dx * 1.0f, textY + dy * 1.0f), c, accentPart);
                    }
                }
                drawList->AddText(titleFont, titleFontSize,
                    ImVec2(accentX, textY), accentCol, accentPart);
            } else {
                // Fallback: plain single-color title
                const ImVec2 titleSize = titleFont->CalcTextSizeA(titleFontSize, FLT_MAX, 0.0f, title);
                drawList->AddText(
                    titleFont, titleFontSize,
                    ImVec2(min.x + 14.0f, min.y + (size.y - titleSize.y) * 0.5f - 1.0f),
                    IM_COL32(235, 235, 235, 255),
                    title
                );
            }
        }

        bool DrawMenuColorCard(const ImVec2& min, const ImVec2& size)
        {
            const float trackHeight = 8.0f;
            const ImVec2 trackMin(min.x + 12.0f, min.y + (size.y - trackHeight) * 0.5f);
            const ImVec2 trackMax(min.x + size.x - 12.0f, trackMin.y + trackHeight);

            ImGui::SetCursorScreenPos(ImVec2(trackMin.x - 2.0f, trackMin.y - 6.0f));
            ImGui::InvisibleButton("##main_runtime_menu_color_slider", ImVec2((trackMax.x - trackMin.x) + 4.0f, 20.0f));
            const bool active = ImGui::IsItemActive();
            const bool hovered = ImGui::IsItemHovered();
            bool changed = false;
            if (active)
            {
                main_runtime_theme::g_menuHue = Normalize(ImGui::GetIO().MousePos.x, trackMin.x, trackMax.x);
                main_runtime_theme::ApplyAccentFromHue();
                changed = true;
            }

            ImDrawList* drawList = ImGui::GetWindowDrawList();
            DrawPanelShell(drawList, min, ImVec2(min.x + size.x, min.y + size.y), 11.0f, hovered || active);

            const float segments = 6.0f;
            const float segmentWidth = (trackMax.x - trackMin.x) / segments;
            drawList->AddRectFilled(
                ImVec2(trackMin.x - 4.0f, trackMin.y - 4.0f),
                ImVec2(trackMax.x + 4.0f, trackMax.y + 4.0f),
                IM_COL32(5, 5, 5, 235),
                5.0f
            );

            for (int i = 0; i < (int)segments; ++i)
            {
                float r0 = 0.0f, g0 = 0.0f, b0 = 0.0f;
                float r1 = 0.0f, g1 = 0.0f, b1 = 0.0f;
                ImGui::ColorConvertHSVtoRGB((float)i / segments, 0.78f, 1.0f, r0, g0, b0);
                ImGui::ColorConvertHSVtoRGB((float)(i + 1) / segments, 0.78f, 1.0f, r1, g1, b1);
                const ImVec2 segMin(trackMin.x + segmentWidth * i, trackMin.y);
                const ImVec2 segMax((i == (int)segments - 1) ? trackMax.x : (segMin.x + segmentWidth + 1.0f), trackMax.y);
                drawList->AddRectFilledMultiColor(
                    segMin,
                    segMax,
                    ImGui::ColorConvertFloat4ToU32(ImVec4(r0, g0, b0, 1.0f)),
                    ImGui::ColorConvertFloat4ToU32(ImVec4(r1, g1, b1, 1.0f)),
                    ImGui::ColorConvertFloat4ToU32(ImVec4(r1, g1, b1, 1.0f)),
                    ImGui::ColorConvertFloat4ToU32(ImVec4(r0, g0, b0, 1.0f))
                );
            }
            drawList->AddRect(trackMin, trackMax, IM_COL32(45, 45, 45, 220), 4.0f, 0, 1.0f);

            const float knobX = ImLerp(trackMin.x, trackMax.x, main_runtime_theme::g_menuHue);
            const float knobHalfWidth = active ? 4.0f : 3.5f;
            const float knobHalfHeight = active ? 8.0f : 7.0f;
            const ImVec2 knobCenter(knobX, (trackMin.y + trackMax.y) * 0.5f);
            const ImVec2 knobMin(knobCenter.x - knobHalfWidth, knobCenter.y - knobHalfHeight);
            const ImVec2 knobMax(knobCenter.x + knobHalfWidth, knobCenter.y + knobHalfHeight);
            drawList->AddRectFilled(
                knobMin - ImVec2(2.0f, 2.0f),
                knobMax + ImVec2(2.0f, 2.0f),
                main_runtime_theme::GetAccentTintU32(0.18f, active ? 0.26f : 0.18f),
                6.0f
            );
            drawList->AddRectFilled(knobMin, knobMax, main_runtime_theme::GetAccentU32(), 4.0f);
            drawList->AddRectFilled(
                ImVec2(knobCenter.x - 1.0f, knobMin.y + 2.0f),
                ImVec2(knobCenter.x + 1.0f, knobMax.y - 2.0f),
                IM_COL32(235, 235, 235, 235),
                1.0f
            );

            return changed;
        }

        bool DrawCloseCard(const ImVec2& min, const ImVec2& size)
        {
            ImGui::SetCursorScreenPos(min);
            ImGui::InvisibleButton("##main_runtime_close_header_card", size);
            const bool hovered = ImGui::IsItemHovered();
            const bool held = ImGui::IsItemActive();
            const bool clicked = ImGui::IsItemClicked();

            ImDrawList* drawList = ImGui::GetWindowDrawList();
            DrawPanelShell(drawList, min, ImVec2(min.x + size.x, min.y + size.y), 11.0f, hovered || held);

            if (hovered || held)
            {
                drawList->AddRectFilled(
                    ImVec2(min.x + 6.0f, min.y + 6.0f),
                    ImVec2(min.x + size.x - 6.0f, min.y + size.y - 6.0f),
                    IM_COL32(255, 255, 255, held ? 16 : 10),
                    8.0f
                );
            }

            ImFont* iconFont = GetIconFont();
            const char* powerIcon = ICON_FA_POWER_OFF;
            const float iconFontSize = hovered || held ? 22.0f : 20.5f;
            const ImVec2 iconSize = MeasureText(iconFont, iconFontSize, powerIcon);
            drawList->AddText(
                iconFont,
                iconFontSize,
                ImVec2(min.x + (size.x - iconSize.x) * 0.5f, min.y + (size.y - iconSize.y) * 0.5f - 1.0f),
                hovered ? main_runtime_theme::GetAccentU32() : IM_COL32(235, 235, 235, 255),
                powerIcon
            );

            return clicked;
        }
    }


    const char* keys[] =
    {
        "-",
        "Mouse 1",
        "Mouse 2",
        "CN",
        "Mouse 3",
        "Mouse 4",
        "Mouse 5",
        "-",
        "Back",
        "Tab",
        "-",
        "-",
        "CLR",
        "Enter",
        "-",
        "-",
        "Shift",
        "CTL",
        "Menu",
        "Pause",
        "Caps Lock",
        "KAN",
        "-",
        "JUN",
        "FIN",
        "KAN",
        "-",
        "Escape",
        "CON",
        "NCO",
        "ACC",
        "MAD",
        "Space",
        "PGU",
        "PGD",
        "End",
        "Home",
        "Left",
        "Up",
        "Right",
        "Down",
        "SEL",
        "PRI",
        "EXE",
        "PRI",
        "INS",
        "Delete",
        "HEL",
        "0",
        "1",
        "2",
        "3",
        "4",
        "5",
        "6",
        "7",
        "8",
        "9",
        "-",
        "-",
        "-",
        "-",
        "-",
        "-",
        "-",
        "A",
        "B",
        "C",
        "D",
        "E",
        "F",
        "G",
        "H",
        "I",
        "J",
        "K",
        "L",
        "M",
        "N",
        "O",
        "P",
        "Q",
        "R",
        "S",
        "T",
        "U",
        "V",
        "W",
        "X",
        "Y",
        "Z",
        "WIN",
        "WIN",
        "APP",
        "-",
        "SLE",
        "Numpad 0",
        "Numpad 1",
        "Numpad 2",
        "Numpad 3",
        "Numpad 4",
        "Numpad 5",
        "Numpad 6",
        "Numpad 7",
        "Numpad 8",
        "Numpad 9",
        "MUL",
        "ADD",
        "SEP",
        "MIN",
        "Delete",
        "DIV",
        "F1",
        "F2",
        "F3",
        "F4",
        "F5",
        "F6",
        "F7",
        "F8",
        "F9",
        "F10",
        "F11",
        "F12",
        "F13",
        "F14",
        "F15",
        "F16",
        "F17",
        "F18",
        "F19",
        "F20",
        "F21",
        "F22",
        "F23",
        "F24",
        "-",
        "-",
        "-",
        "-",
        "-",
        "-",
        "-",
        "-",
        "NUM",
        "SCR",
        "EQU",
        "MAS",
        "TOY",
        "OYA",
        "OYA",
        "-",
        "-",
        "-",
        "-",
        "-",
        "-",
        "-",
        "-",
        "-",
        "Shift",
        "Shift",
        "Ctrl",
        "Ctrl",
        "Alt",
        "Alt"
    };

#include <string>

    struct key_state
    {
        ImVec4 background, text;
        bool active = false;
        bool hovered = false;
        float alpha = 0.f;
    };

    static ImVec4 MixColor(const ImVec4& a, const ImVec4& b, float t)
    {
        return ImVec4(
            ImLerp(a.x, b.x, t),
            ImLerp(a.y, b.y, t),
            ImLerp(a.z, b.z, t),
            ImLerp(a.w, b.w, t)
        );
    }

    static ImVec4 AccentLift(float toward_white, float alpha = 1.0f)
    {
        ImVec4 col = MixColor(c::accent, ImVec4(1.0f, 1.0f, 1.0f, c::accent.w), ImClamp(toward_white, 0.0f, 1.0f));
        col.w *= alpha;
        return col;
    }

    static ImVec4 AccentShade(float toward_black, float alpha = 1.0f)
    {
        ImVec4 col = MixColor(c::accent, ImVec4(0.0f, 0.0f, 0.0f, c::accent.w), ImClamp(toward_black, 0.0f, 1.0f));
        col.w *= alpha;
        return col;
    }

    static ImVec4 WithAlpha(const ImVec4& col, float alpha)
    {
        ImVec4 out = col;
        out.w *= alpha;
        return out;
    }

    static bool VisibleLabelEquals(const char* label, const char* label_end, const char* target)
    {
        if (label == nullptr || target == nullptr)
            return false;
        if (label_end == nullptr)
            label_end = FindRenderedTextEnd(label);
        const int len = (int)(label_end - label);
        const int target_len = (int)strlen(target);
        return len == target_len && ImStrnicmp(label, target, len) == 0;
    }

    static const char* GetChildCapIcon(const char* label, const char* label_end)
    {
        if (VisibleLabelEquals(label, label_end, "ESP")) return ICON_FA_EYE;
        if (VisibleLabelEquals(label, label_end, "ESP OPTIONS")) return ICON_FA_SLIDERS_H;
        if (VisibleLabelEquals(label, label_end, "AIMBOT")) return ICON_FA_CROSSHAIRS;
        if (VisibleLabelEquals(label, label_end, "COMBAT OPTIONS")) return ICON_FA_SLIDERS_H;
        if (VisibleLabelEquals(label, label_end, "MEMORY HACKS")) return ICON_FA_BOLT;
        if (VisibleLabelEquals(label, label_end, "UTILITY")) return ICON_FA_WRENCH;
        if (VisibleLabelEquals(label, label_end, "LICENSE INFO")) return ICON_FA_UNLOCK_ALT;
        if (VisibleLabelEquals(label, label_end, "LOGO SETTINGS")) return ICON_FA_STAR;
        if (VisibleLabelEquals(label, label_end, "CONFIG MANAGEMENT")) return ICON_FA_COG;
        if (VisibleLabelEquals(label, label_end, "ENHANCEMENT")) return ICON_FA_SLIDERS_H;
        if (VisibleLabelEquals(label, label_end, "SKINS")) return ICON_FA_TH_LIST;
        if (VisibleLabelEquals(label, label_end, "MISC")) return ICON_FA_SLIDERS_H;
        if (VisibleLabelEquals(label, label_end, "WEAPON SKINS")) return ICON_FA_TH_LIST;
        if (VisibleLabelEquals(label, label_end, "CHARACTER SKINS")) return ICON_FA_TH_LIST;
        if (VisibleLabelEquals(label, label_end, "DEADBOX SKINS")) return ICON_FA_TH_LIST;
        return nullptr;
    }

    bool Keybind(const char* label, int* key, int* mode)
    {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems) return false;

        ImGuiContext& g = *GImGui;
        ImGuiIO& io = g.IO;
        const ImGuiStyle& style = g.Style;

        const ImGuiID id = window->GetID(label);
        const float width = (GetContentRegionMax().x - style.WindowPadding.x);
        const float height = 32 * c::scale;

        const ImRect rect(window->DC.CursorPos, window->DC.CursorPos + ImVec2(width, height));

        ItemSize(ImRect(rect.Min, rect.Max));
        if (!ImGui::ItemAdd(rect, id)) return false;

        char buf_display[64] = "None";

        bool value_changed = false;
        int k = *key;

        std::string active_key = "";
        active_key += keys[*key];

        if (*key != 0 && g.ActiveId != id) {
            strcpy(buf_display, active_key.c_str());
        }
        else if (g.ActiveId == id) {
            strcpy(buf_display, "...");
        }

        const ImVec2 label_size = CalcTextSize(buf_display, NULL, true);

        ImRect clickable(ImVec2(rect.Max.x - 20 * c::scale - label_size.x, rect.Min.y), rect.Max);
        bool hovered = ItemHoverable(clickable, id, NULL);

        static std::map<ImGuiID, key_state> anim;
        auto it_anim = anim.find(id);

        if (it_anim == anim.end())
        {
            anim.insert({ id, key_state() });
            it_anim = anim.find(id);
        }

        it_anim->second.text = ImLerp(it_anim->second.text, g.ActiveId == id ? c::text::text_active : hovered ? c::text::text_hov : c::text::text, ImGui::GetIO().DeltaTime * 6.f);

        window->DrawList->AddRectFilled(clickable.Min, clickable.Max, GetColorU32(c::elements::background), c::elements::rounding * c::scale);

        window->DrawList->AddText(rect.Min + ImVec2(0, (height - CalcTextSize(label).y) / 2), GetColorU32(it_anim->second.text), label);

        PushStyleColor(ImGuiCol_Text, c::text::text);
        PushFont(font::icomoon_page);
        RenderTextClipped(clickable.Min - ImVec2(30 * c::scale, 0), clickable.Max, "j", NULL, NULL, ImVec2(0.0f, 0.5f));
        PopFont();
        PopStyleColor();

        PushStyleColor(ImGuiCol_Text, c::text::text_active);
        RenderTextClipped(clickable.Min, clickable.Max, buf_display, NULL, &label_size, ImVec2(0.5f, 0.5f));
        PopStyleColor();

        if (hovered && io.MouseClicked[0])
        {
            if (g.ActiveId != id) {

                memset(io.MouseDown, 0, sizeof(io.MouseDown));
                memset(io.KeysDown, 0, sizeof(io.KeysDown));
                *key = 0;
            }
            ImGui::SetActiveID(id, window);
            ImGui::FocusWindow(window);
        }
        else if (io.MouseClicked[0]) {

            if (g.ActiveId == id)
                ImGui::ClearActiveID();
        }

        if (g.ActiveId == id) {
            for (auto i = 0; i < 5; i++) {
                if (io.MouseDown[i]) {
                    switch (i) {
                    case 0:
                        k = 0x01;
                        break;
                    case 1:
                        k = 0x02;
                        break;
                    case 2:
                        k = 0x04;
                        break;
                    case 3:
                        k = 0x05;
                        break;
                    case 4:
                        k = 0x06;
                        break;
                    }
                    value_changed = true;
                    ImGui::ClearActiveID();
                }
            }
            if (!value_changed) {
                for (auto i = 0x08; i <= 0xA5; i++) {
                    if (io.KeysDown[i]) {
                        k = i;
                        value_changed = true;
                        ImGui::ClearActiveID();
                    }
                }
            }

            if (IsKeyPressedMap(ImGuiKey_Escape)) {
                *key = 0;
                ImGui::ClearActiveID();
            }
            else {
                *key = k;
            }
        }

        return value_changed;
    }

    bool ChildEx(const char* name, ImGuiID id, const ImVec2& size_arg, bool cap, ImGuiWindowFlags flags)
    {
        ImGuiContext& g = *GImGui;
        ImGuiWindow* parent_window = g.CurrentWindow;

        flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_ChildWindow;
        flags |= (parent_window->Flags & ImGuiWindowFlags_NoMove);

        const ImVec2 content_avail = GetContentRegionAvail();
        ImVec2 size = ImTrunc(size_arg);
        const int auto_fit_axises = ((size.x == 0.0f) ? (1 << ImGuiAxis_X) : 0x00) | ((size.y == 0.0f) ? (1 << ImGuiAxis_Y) : 0x00);
        if (size.x <= 0.0f) size.x = ImMax(content_avail.x + size.x, 4.0f * c::scale);
        if (size.y <= 0.0f) size.y = ImMax(content_avail.y + size.y, 4.0f * c::scale);

        // Card look follows the reference menu: rounded panel, a title row with
        // an accent underline, and the active theme's colours instead of the
        // hard-coded near-black that clashed with the zenin charcoal palette.
        const float rounding = 8.0f * c::scale;
        const float cap_height = cap ? 30.0f * c::scale : 0.0f;
        const ImVec2 panel_min = parent_window->DC.CursorPos;
        const ImVec2 panel_max = panel_min + size;
        SetNextWindowPos(panel_min + ImVec2(0.0f, cap_height));
        SetNextWindowSize(size - ImVec2(0.0f, cap_height));

        ImDrawList* draw = GetWindowDrawList();
        const ImU32 card_bg     = GetColorU32(ImVec4(c::child::background.x, c::child::background.y, c::child::background.z, 1.0f));
        const ImU32 card_border = GetColorU32(ImVec4(c::widget::outlinecolor.x, c::widget::outlinecolor.y, c::widget::outlinecolor.z, 0.85f));
        draw->AddRectFilled(panel_min, panel_max, card_bg, rounding);
        draw->AddRect(panel_min, panel_max, card_border, rounding, 0, 1.0f);

        if (cap) {
            const ImVec2 cap_max(panel_max.x, panel_min.y + cap_height);
            draw->AddRectFilled(panel_min, ImVec2(panel_max.x, cap_max.y),
                                GetColorU32(ImVec4(c::child::cap.x, c::child::cap.y, c::child::cap.z, 1.0f)),
                                rounding, ImDrawFlags_RoundCornersTop);
            draw->AddLine(ImVec2(panel_min.x + 12.0f, cap_max.y), ImVec2(panel_max.x - 12.0f, cap_max.y),
                          GetColorU32(ImVec4(c::separator.x, c::separator.y, c::separator.z, 0.75f)), 1.0f);
            // Reference card header: a 1px accent hairline across the very top of
            // the cap (the reference peeks its accent strip out from under the
            // cap) plus the short accent underline below the title.
            draw->AddRectFilled(panel_min,
                                ImVec2(panel_max.x, panel_min.y + 1.0f),
                                GetColorU32(c::accent),
                                rounding, ImDrawFlags_RoundCornersTop);
            draw->AddRectFilled(ImVec2(panel_min.x + 12.0f, cap_max.y - 1.6f),
                                ImVec2(panel_min.x + 12.0f + 34.0f, cap_max.y - 0.4f),
                                GetColorU32(c::accent), 1.0f);

            const char* name_end = name ? FindRenderedTextEnd(name) : nullptr;
            if (name != nullptr && name_end != name) {
                const ImVec2 title_size = CalcTextSize(name, name_end);
                float text_x = 12.0f * c::scale;
                if (F107 != nullptr) {
                    const char* icon = GetChildCapIcon(name, name_end);
                    if (icon != nullptr) {
                        const float icon_size = 10.8f * c::scale;
                        const ImVec2 icon_text_size = F107->CalcTextSizeA(icon_size, FLT_MAX, 0.0f, icon);
                        draw->AddText(
                            F107,
                            icon_size,
                            panel_min + ImVec2(text_x, ImMax(6.0f * c::scale, (cap_height - icon_text_size.y) * 0.5f - 1.0f * c::scale)),
                            GetColorU32(c::accent),
                            icon
                        );
                        text_x += icon_text_size.x + 7.0f * c::scale;
                    }
                }
                // Card titles use the reference's bold cut (inter_bold, 17px)
                // rather than the body font.
                ImFont* card_font = GetCardTitleFont();
                const float card_size = (card_font != nullptr) ? card_font->FontSize : 17.0f;
                const ImVec2 card_text_size = MeasureText(card_font, card_size, name);
                draw->AddText(
                    card_font,
                    card_size,
                    panel_min + ImVec2(text_x, ImMax(5.0f * c::scale, (cap_height - (card_text_size.y > 0.0f ? card_text_size.y : title_size.y)) * 0.5f - 1.0f * c::scale)),
                    GetColorU32(c::text::text_active),
                    name,
                    name_end
                );
            }
        }

        const char* temp_window_name;

        if (name) ImFormatStringToTempBuffer(&temp_window_name, NULL, "%s/%s_%08X", parent_window->Name, name, id);

        else ImFormatStringToTempBuffer(&temp_window_name, NULL, "%s/%08X", parent_window->Name, id);

        const float backup_border_size = g.Style.ChildBorderSize;

        bool ret = Begin(temp_window_name, NULL, flags | ImGuiWindowFlags_NoBackground);

        ImGuiWindow* child_window = g.CurrentWindow;
        child_window->ChildId = id;
        child_window->AutoFitChildAxises = (ImS8)auto_fit_axises;

        if (child_window->BeginCount == 1) parent_window->DC.CursorPos = child_window->Pos;

        const ImGuiID temp_id_for_activation = ImHashStr("##Child", 0, id);
        if (g.ActiveId == temp_id_for_activation) ClearActiveID();

        if (g.NavActivateId == id && !(flags & ImGuiWindowFlags_NavFlattened) && (child_window->DC.NavLayersActiveMask != 0 || child_window->DC.NavWindowHasScrollY))
        {
            FocusWindow(child_window);
            NavInitWindow(child_window, false);
            SetActiveID(temp_id_for_activation, child_window);
            g.ActiveIdSource = g.NavInputSource;
        }
        return ret;
    }

    bool Child(const char* str_id, const ImVec2& size_arg, bool cap, ImGuiWindowFlags extra_flags)
    {
        ImGuiWindow* window = GetCurrentWindow();

        PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(c::child::padding * c::scale, c::child::padding * c::scale));
        PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(c::child::spacing * c::scale, c::child::spacing * c::scale));

        return ChildEx(str_id, window->GetID(str_id), size_arg, cap, extra_flags | ImGuiWindowFlags_AlwaysUseWindowPadding);
    }

    bool ChildID(ImGuiID id, const ImVec2& size_arg, bool cap, ImGuiWindowFlags extra_flags)
    {
        IM_ASSERT(id != 0);
        return ChildEx(NULL, id, size_arg, cap, extra_flags);
    }

    void EndChild()
    {
        ImGuiContext& g = *GImGui;
        ImGuiWindow* window = g.CurrentWindow;
        PopStyleVar(2);

        IM_ASSERT(g.WithinEndChild == false);
        IM_ASSERT(window->Flags & ImGuiWindowFlags_ChildWindow);

        g.WithinEndChild = true;
        if (window->BeginCount > 1)
        {
            End();
        }
        else
        {
            ImVec2 sz = window->Size;

            if (window->AutoFitChildAxises & (1 << ImGuiAxis_X)) sz.x = ImMax(4.0f * c::scale, sz.x);
            if (window->AutoFitChildAxises & (1 << ImGuiAxis_Y)) sz.y = ImMax(4.0f * c::scale, sz.y);

            End();

            ImGuiWindow* parent_window = g.CurrentWindow;
            ImRect bb(parent_window->DC.CursorPos, parent_window->DC.CursorPos + sz);
            ItemSize(sz);
            if ((window->DC.NavLayersActiveMask != 0 || window->DC.NavWindowHasScrollY) && !(window->Flags & ImGuiWindowFlags_NavFlattened))
            {
                ItemAdd(bb, window->ChildId);
            }
            else
            {
                ItemAdd(bb, 0);

                if (window->Flags & ImGuiWindowFlags_NavFlattened) parent_window->DC.NavLayersActiveMaskNext |= window->DC.NavLayersActiveMaskNext;
            }
            if (g.HoveredWindow == window) g.LastItemData.StatusFlags |= ImGuiItemStatusFlags_HoveredWindow;
        }
        g.WithinEndChild = false;
        g.LogLinePosY = -FLT_MAX;
    }

    void BeginGroup()
    {
        ImGuiContext& g = *GImGui;
        ImGuiWindow* window = g.CurrentWindow;

        g.GroupStack.resize(g.GroupStack.Size + 1);
        ImGuiGroupData& group_data = g.GroupStack.back();
        group_data.WindowID = window->ID;
        group_data.BackupCursorPos = window->DC.CursorPos;
        group_data.BackupCursorMaxPos = window->DC.CursorMaxPos;
        group_data.BackupIndent = window->DC.Indent;
        group_data.BackupGroupOffset = window->DC.GroupOffset;
        group_data.BackupCurrLineSize = window->DC.CurrLineSize;
        group_data.BackupCurrLineTextBaseOffset = window->DC.CurrLineTextBaseOffset;
        group_data.BackupActiveIdIsAlive = g.ActiveIdIsAlive;
        group_data.BackupHoveredIdIsAlive = g.HoveredId != 0;
        group_data.BackupActiveIdPreviousFrameIsAlive = g.ActiveIdPreviousFrameIsAlive;
        group_data.EmitItem = true;

        window->DC.GroupOffset.x = window->DC.CursorPos.x - window->Pos.x - window->DC.ColumnsOffset.x;
        window->DC.Indent = window->DC.GroupOffset;
        window->DC.CursorMaxPos = window->DC.CursorPos;
        window->DC.CurrLineSize = ImVec2(0.0f, 0.0f);
        if (g.LogEnabled) g.LogLinePosY = -FLT_MAX;
    }

    void EndGroup()
    {
        ImGuiContext& g = *GImGui;
        ImGuiWindow* window = g.CurrentWindow;
        IM_ASSERT(g.GroupStack.Size > 0);

        ImGuiGroupData& group_data = g.GroupStack.back();
        IM_ASSERT(group_data.WindowID == window->ID);

        if (window->DC.IsSetPos) ErrorCheckUsingSetCursorPosToExtendParentBoundaries();

        ImRect group_bb(group_data.BackupCursorPos, ImMax(window->DC.CursorMaxPos, group_data.BackupCursorPos));

        window->DC.CursorPos = group_data.BackupCursorPos;
        window->DC.CursorMaxPos = ImMax(group_data.BackupCursorMaxPos, window->DC.CursorMaxPos);
        window->DC.Indent = group_data.BackupIndent;
        window->DC.GroupOffset = group_data.BackupGroupOffset;
        window->DC.CurrLineSize = group_data.BackupCurrLineSize;
        window->DC.CurrLineTextBaseOffset = group_data.BackupCurrLineTextBaseOffset;
        if (g.LogEnabled) g.LogLinePosY = -FLT_MAX;

        if (!group_data.EmitItem)
        {
            g.GroupStack.pop_back();
            return;
        }

        window->DC.CurrLineTextBaseOffset = ImMax(window->DC.PrevLineTextBaseOffset, group_data.BackupCurrLineTextBaseOffset);
        ItemSize(group_bb.GetSize());
        ItemAdd(group_bb, 0, NULL, ImGuiItemFlags_NoTabStop);

        const bool group_contains_curr_active_id = (group_data.BackupActiveIdIsAlive != g.ActiveId) && (g.ActiveIdIsAlive == g.ActiveId) && g.ActiveId;
        const bool group_contains_prev_active_id = (group_data.BackupActiveIdPreviousFrameIsAlive == false) && (g.ActiveIdPreviousFrameIsAlive == true);
        if (group_contains_curr_active_id) g.LastItemData.ID = g.ActiveId;
        else if (group_contains_prev_active_id) g.LastItemData.ID = g.ActiveIdPreviousFrame;
        g.LastItemData.Rect = group_bb;

        const bool group_contains_curr_hovered_id = (group_data.BackupHoveredIdIsAlive == false) && g.HoveredId != 0;
        if (group_contains_curr_hovered_id) g.LastItemData.StatusFlags |= ImGuiItemStatusFlags_HoveredWindow;

        if (group_contains_curr_active_id && g.ActiveIdHasBeenEditedThisFrame) g.LastItemData.StatusFlags |= ImGuiItemStatusFlags_Edited;

        g.LastItemData.StatusFlags |= ImGuiItemStatusFlags_HasDeactivated;
        if (group_contains_prev_active_id && g.ActiveId != g.ActiveIdPreviousFrame) g.LastItemData.StatusFlags |= ImGuiItemStatusFlags_Deactivated;

        g.GroupStack.pop_back();
    }

    void Separator_line()
    {
        ImVec2 pos = GetCursorScreenPos();
        GetWindowDrawList()->AddRectFilled(pos, pos + ImVec2(GetContentRegionMax().x - GetStyle().WindowPadding.x, 1 * c::scale), GetColorU32(c::separator));
        SetCursorScreenPos(ImVec2(pos.x, pos.y + 1 * c::scale + 2));
    }

    void SeparatorEx(ImGuiSeparatorFlags flags, float thickness)
    {
        ImGuiWindow* window = GetCurrentWindow();
        if (window->SkipItems) return;

        ImGuiContext& g = *GImGui;
        IM_ASSERT(ImIsPowerOfTwo(flags & (ImGuiSeparatorFlags_Horizontal | ImGuiSeparatorFlags_Vertical)));
        IM_ASSERT(thickness > 0.0f);

        if (flags & ImGuiSeparatorFlags_Vertical)
        {
            float y1 = window->DC.CursorPos.y;
            float y2 = window->DC.CursorPos.y + window->DC.CurrLineSize.y;
            const ImRect bb(ImVec2(window->DC.CursorPos.x, y1 + (GetStyle().ItemSpacing.y / 2) ), ImVec2(window->DC.CursorPos.x + thickness * c::scale, y2 - (GetStyle().ItemSpacing.y / 2) ));

            ItemSize(ImVec2(thickness * c::scale, 0.0f));
            if (!ItemAdd(bb, 0)) return;

            window->DrawList->AddRectFilled(bb.Min, bb.Max, GetColorU32(c::child::background));

            ImGui::SameLine();
        }
        else if (flags & ImGuiSeparatorFlags_Horizontal)
        {
            float x1 = window->Pos.x;
            float x2 = window->Pos.x + window->Size.x;

            if (g.GroupStack.Size > 0 && g.GroupStack.back().WindowID == window->ID) x1 += window->DC.Indent.x;

            if (ImGuiTable* table = g.CurrentTable)
            {
                x1 = table->Columns[table->CurrentColumn].MinX;
                x2 = table->Columns[table->CurrentColumn].MaxX;
            }

            ImGuiOldColumns* columns = (flags & ImGuiSeparatorFlags_SpanAllColumns) ? window->DC.CurrentColumns : NULL;
            if (columns) PushColumnsBackground();

            const float thickness_for_layout = (thickness == 1.0f) ? 0.0f : thickness * c::scale;
            const ImRect bb(ImVec2(x1 + GetStyle().WindowPadding.x, window->DC.CursorPos.y), ImVec2(x2 - GetStyle().WindowPadding.x, window->DC.CursorPos.y + thickness * c::scale));

            ItemSize(ImVec2(0.0f, thickness_for_layout));

            if (ItemAdd(bb, 0))
            {
                window->DrawList->AddRectFilled(bb.Min, bb.Max, GetColorU32(c::separator));
            }
            if (columns)
            {
                PopColumnsBackground();
                columns->LineMinY = window->DC.CursorPos.y;
            }
        }
    }

    void Separator()
    {
        ImGuiContext& g = *GImGui;
        ImGuiWindow* window = g.CurrentWindow;
        if (window->SkipItems) return;

        ImGuiSeparatorFlags flags = (window->DC.LayoutType == ImGuiLayoutType_Horizontal) ? ImGuiSeparatorFlags_Vertical : ImGuiSeparatorFlags_Horizontal;
        flags |= ImGuiSeparatorFlags_SpanAllColumns;
        SeparatorEx(flags, 1.0f);
    }

    struct theme_state
    {
        ImVec4 background;
        float smooth_swap, alpha_line, line_size;
    };

    bool ThemeButton(const char* id_theme, bool dark, const ImVec2& size_arg)
    {
        ImGuiWindow* window = GetCurrentWindow();

        if (window->SkipItems) return false;

        ImGuiContext& g = *GImGui;
        const ImGuiStyle& style = g.Style;
        const ImGuiID id = window->GetID(id_theme);
        const ImVec2 label_size = CalcTextSize(id_theme, NULL, true), pos = window->DC.CursorPos;
        const float control_scale = c::scale * c::widget_scale;

        ImVec2 size = CalcItemSize(size_arg, label_size.x + 16.0f * control_scale, label_size.y + 8.0f * control_scale);
        if (size_arg.y > 0.0f)
            size.y = ImMax(22.0f * control_scale, size.y * c::widget_scale - 4.0f * control_scale);
        else
            size.y = ImMax(size.y, 22.0f * control_scale);

        const ImRect bb(pos, pos + size);

        ItemSize(size, 0.f);
        if (!ItemAdd(bb, id)) return false;

        bool hovered, held, pressed = ButtonBehavior(bb, id, &hovered, &held, NULL);

        static std::map<ImGuiID, theme_state> anim;
        auto it_anim = anim.find(id);

        if (it_anim == anim.end())
        {
            anim.insert({ id, theme_state() });
            it_anim = anim.find(id);
        }

        it_anim->second.background = ImLerp(it_anim->second.background, dark || hovered ? c::page::background_active : c::page::background, g.IO.DeltaTime * 6.f);

        it_anim->second.alpha_line = ImLerp(it_anim->second.alpha_line, dark ? 1.f : 0.f, g.IO.DeltaTime * 6.f);
        it_anim->second.line_size = ImLerp(it_anim->second.line_size, dark ? (size_arg.x / 4) : (size_arg.x / 2), g.IO.DeltaTime * 6.f);

        it_anim->second.smooth_swap = ImLerp(it_anim->second.smooth_swap, dark ? 26.f * c::scale : 0, g.IO.DeltaTime * 12.f);

        GetWindowDrawList()->AddRectFilled(bb.Min, bb.Max, GetColorU32(it_anim->second.background), c::page::rounding * c::scale);

        PushClipRect(bb.Min, bb.Max, true);

        PushFont(font::icomoon_page);
        GetWindowDrawList()->AddText(ImVec2(bb.Min.x + (size_arg.x - CalcTextSize("k").x) / 2, bb.Max.y - CalcTextSize("k").y - (size.y - CalcTextSize("k").y) / 2 + it_anim->second.smooth_swap), GetColorU32(c::accent), "k");
        GetWindowDrawList()->AddText(ImVec2(bb.Min.x + (size_arg.x - CalcTextSize("a").x) / 2, bb.Max.y - CalcTextSize("a").y - (size.y - CalcTextSize("a").y) / 2 - 25 * c::scale + it_anim->second.smooth_swap), GetColorU32(c::accent), "a");
        PopFont();

        PopClipRect();

        return pressed;
    }

    struct button_state
    {
        ImVec4 background, text;
    };

    bool Button(const char* label, const ImVec2& size_arg)
    {
        ImGuiWindow* window = GetCurrentWindow();

        if (window->SkipItems) return false;

        ImGuiContext& g = *GImGui;
        const ImGuiID id = window->GetID(label);
        const char* label_end = FindRenderedTextEnd(label);
        const ImVec2 label_size = CalcTextSize(label, label_end, true), pos = window->DC.CursorPos;
        const float control_scale = c::scale * c::widget_scale;

        ImVec2 size = CalcItemSize(size_arg, label_size.x + 16.0f * control_scale, label_size.y + 10.0f * control_scale);
        if (size_arg.y > 0.0f)
            size.y = ImMax(size.y * c::widget_scale, 22.0f * control_scale);
        else
            size.y = ImMax(size.y, 22.0f * control_scale);

        const ImRect bb(pos, pos + size);

        ItemSize(size, 0.f);
        if (!ItemAdd(bb, id)) return false;

        bool hovered, held, pressed = ButtonBehavior(bb, id, &hovered, &held, NULL);

        static std::map<ImGuiID, button_state> anim;
        auto it_anim = anim.find(id);

        if (it_anim == anim.end())
        {
            anim.insert({ id, button_state() });
            it_anim = anim.find(id);
        }

        const ImVec4 target_background = held
            ? c::button::background_active
            : hovered ? c::button::background_hovered : c::button::background;
        const ImVec4 target_text = held
            ? c::text::text_active
            : hovered ? c::accent : c::text::text_active;

        it_anim->second.background = ImLerp(it_anim->second.background, target_background, g.IO.DeltaTime * 10.0f);
        it_anim->second.text = ImLerp(it_anim->second.text, target_text, g.IO.DeltaTime * 10.0f);

        const float rounding = 6.0f * c::scale;
        GetWindowDrawList()->AddRectFilled(bb.Min, bb.Max, GetColorU32(it_anim->second.background), rounding);
        GetWindowDrawList()->AddRect(
            bb.Min,
            bb.Max,
            GetColorU32(hovered || held ? c::accent : c::button::outline, hovered || held ? 0.95f : 0.72f),
            rounding,
            0,
            1.0f * c::scale
        );

        PushClipRect(bb.Min, bb.Max, true);

        const bool use_icon_font = (label_end == label + 1 && font::icomoon_page != nullptr);
        ImVec2 text_size;
        if (use_icon_font)
        {
            PushFont(font::icomoon_page);
            text_size = CalcTextSize(label, label_end);
            PopFont();
        }
        else
        {
            text_size = CalcTextSize(label, label_end);
        }

        ImVec2 text_pos = ImVec2(
            bb.Min.x + (size.x - text_size.x) * 0.5f,
            bb.Min.y + (size.y - text_size.y) * 0.5f
        );

        if (use_icon_font) PushFont(font::icomoon_page);
        GetWindowDrawList()->AddText(text_pos, GetColorU32(it_anim->second.text), label, label_end);
        if (use_icon_font) PopFont();

        PopClipRect();

        return pressed;
    }

    struct page_state
    {
        ImVec4 background, text;
        float smooth_swap, alpha_line, line_size;
    };

    bool Page(bool selected, const char* icon, const ImVec2& size_arg, bool allow_icon_font, bool show_active_line)
    {
        ImGuiWindow* window = GetCurrentWindow();

        if (window->SkipItems) return false;

        ImGuiContext& g = *GImGui;
        const ImGuiStyle& style = g.Style;
        const ImGuiID id = window->GetID(icon);
        const ImVec2 label_size = CalcTextSize(icon, NULL, true), pos = window->DC.CursorPos;

        ImVec2 size = CalcItemSize(size_arg, label_size.x, label_size.y);

        const ImRect bb(pos, pos + size);

        ItemSize(size, 0.f);
        if (!ItemAdd(bb, id)) return false;

        bool hovered, held, pressed = ButtonBehavior(bb, id, &hovered, &held, NULL);

        static std::map<ImGuiID, page_state> anim;
        auto it_anim = anim.find(id);

        if (it_anim == anim.end())
        {
            anim.insert({ id, page_state() });
            it_anim = anim.find(id);
        }

        it_anim->second.background = ImLerp(it_anim->second.background, selected || hovered ? c::page::background_active : c::page::background, g.IO.DeltaTime * 6.f);
        it_anim->second.text = ImLerp(it_anim->second.text, selected ? c::accent : hovered ? c::page::text_hov : c::page::text, g.IO.DeltaTime * 14.f);

        it_anim->second.alpha_line = ImLerp(it_anim->second.alpha_line, selected ? 1.f : 0.f, g.IO.DeltaTime * 6.f);
        it_anim->second.line_size = ImLerp(it_anim->second.line_size, selected ? (size.x / 4) : (size.x / 2), g.IO.DeltaTime * 6.f);

        it_anim->second.smooth_swap = ImLerp(it_anim->second.smooth_swap, selected ? 26.f * c::scale : 0, g.IO.DeltaTime * 12.f);

        GetWindowDrawList()->AddRectFilled(bb.Min, bb.Max, GetColorU32(it_anim->second.background), c::page::rounding * c::scale);

        if (show_active_line)
            GetWindowDrawList()->AddRectFilled(bb.Min + ImVec2(+it_anim->second.line_size, (size_arg.y + 9 * c::scale)), bb.Max + ImVec2(-it_anim->second.line_size, 15 * c::scale), GetColorU32(c::accent, it_anim->second.alpha_line), c::page::rounding * c::scale, ImDrawFlags_RoundCornersTop);

        PushClipRect(bb.Min, bb.Max, true);

        const bool use_icon_font = allow_icon_font && (icon && icon[0] != '\0' && icon[1] == '\0' && font::icomoon_page != nullptr);
        ImVec2 text_size;
        if (use_icon_font)
        {
            PushFont(font::icomoon_page);
            text_size = CalcTextSize(icon);
            PopFont();
        }
        else
        {
            text_size = CalcTextSize(icon);
        }

        ImVec2 text_pos_1 = ImVec2(
            bb.Min.x + (size.x - text_size.x) * 0.5f,
            bb.Min.y + (size.y - text_size.y) * 0.5f + it_anim->second.smooth_swap
        );
        ImVec2 text_pos_2 = ImVec2(
            bb.Min.x + (size.x - text_size.x) * 0.5f,
            bb.Min.y + (size.y - text_size.y) * 0.5f - 25 * c::scale + it_anim->second.smooth_swap
        );

        if (use_icon_font) PushFont(font::icomoon_page);
        GetWindowDrawList()->AddText(text_pos_1, GetColorU32(it_anim->second.text), icon);
        GetWindowDrawList()->AddText(text_pos_2, GetColorU32(it_anim->second.text), icon);
        if (use_icon_font) PopFont();

        PopClipRect();

        return pressed;
    }

    struct check_state
    {
        ImVec4 background, text;
        float alpha_mark;
        float hover_t;
        float hold_t;
        float box_scale;
    };

    bool Checkbox(const char* label, bool* v)
    {
        // Compact reference-style row: label on the left, a 36x16 sliding toggle
        // on the right with an accent fill, soft glow and a tinted label.
        ImGuiWindow* window = GetCurrentWindow();
        if (window->SkipItems) return false;

        ImGuiContext& g = *GImGui;
        const ImGuiStyle& style = g.Style;
        const ImGuiID id = window->GetID(label);
        const char* label_end = FindRenderedTextEnd(label);
        const ImVec2 label_size = CalcTextSize(label, label_end, true);
        const float row_h = 22.0f;
        const ImVec2 pos = window->DC.CursorPos;
        const float w = GetContentRegionMax().x - style.WindowPadding.x;

        const ImRect total_bb(pos, pos + ImVec2(w, row_h));
        ItemSize(total_bb, 0.0f);
        if (!ItemAdd(total_bb, id)) return false;

        bool hovered, held, pressed = ButtonBehavior(total_bb, id, &hovered, &held);

        static std::map<ImGuiID, check_state> anim;
        auto it_anim = anim.find(id);
        if (it_anim == anim.end())
        {
            anim.insert({ id, check_state() });
            it_anim = anim.find(id);
            it_anim->second.alpha_mark = *v ? 1.0f : 0.0f;
        }

        if (pressed)
        {
            *v = !(*v);
            MarkItemEdited(id);
        }

        it_anim->second.alpha_mark = ImLerp(it_anim->second.alpha_mark, *v ? 1.f : 0.f, g.IO.DeltaTime * 12.0f);
        it_anim->second.hover_t = ImLerp(it_anim->second.hover_t, hovered ? 1.0f : 0.0f, g.IO.DeltaTime * 10.0f);
        if (hovered) SetMouseCursor(ImGuiMouseCursor_Hand);

        const float track_w = 36.0f;
        const float track_h = 14.0f;   // reference pill is 12px inside an 18px row
        const float knob_r  = 8.0f;    // reference knob is wider than the track
        const ImVec2 track_min(total_bb.Max.x - track_w, total_bb.Min.y + (row_h - track_h) * 0.5f);
        const ImVec2 track_max(track_min.x + track_w, track_min.y + track_h);
        const float travel = track_w - track_h;
        const float knob_x = track_min.x + track_h * 0.5f + travel * it_anim->second.alpha_mark;
        const ImVec2 knob_c(knob_x, (track_min.y + track_max.y) * 0.5f);

        const float on = it_anim->second.alpha_mark;
        // Palette straight from the reference toggle: off track is
        // element::filling, on track is the accent, the knob flips between
        // element::circle_mark and the near-black mark colour.
        const ImVec4 off_track(c::checkbox::background_off.x, c::checkbox::background_off.y, c::checkbox::background_off.z, 1.0f);
        const ImVec4 on_track(c::accent.x, c::accent.y, c::accent.z, 1.0f);
        const ImVec4 track_color = MixColor(off_track, on_track, on);
        const ImVec4 off_knob(c::checkbox::circle_inactive.x, c::checkbox::circle_inactive.y, c::checkbox::circle_inactive.z, 1.0f);
        const ImVec4 on_knob(c::checkbox::mark.x, c::checkbox::mark.y, c::checkbox::mark.z, 1.0f);
        const ImVec4 knob_color = MixColor(off_knob, on_knob, on);
        const ImVec4 off_label(c::text::text.x, c::text::text.y, c::text::text.z, 1.0f);
        const ImVec4 text_color = MixColor(off_label, c::text::text_active, on);

        ImDrawList* dl = GetWindowDrawList();
        if (on > 0.02f)
            dl->AddRectFilled(ImVec2(track_min.x - 2.5f, track_min.y - 2.5f),
                              ImVec2(track_max.x + 2.5f, track_max.y + 2.5f),
                              GetColorU32(ImVec4(c::accent.x, c::accent.y, c::accent.z, 0.20f * on)),
                              (track_h + 5.0f) * 0.5f);
        dl->AddRectFilled(track_min, track_max, GetColorU32(track_color), track_h * 0.5f);
        // Reference "on" treatment: the knob gets the same soft shadow circle the
        // reference draws around a lit toggle knob.
        if (on > 0.02f)
            dl->AddShadowCircle(knob_c, knob_r,
                                GetColorU32(ImVec4(c::accent.x, c::accent.y, c::accent.z, 0.55f * on)),
                                30.0f, ImVec2(0.0f, 0.0f), 0, 30);
        dl->AddCircleFilled(knob_c, knob_r, GetColorU32(knob_color), 26);

        dl->AddText(ImVec2(total_bb.Min.x, total_bb.Min.y + (row_h - label_size.y) * 0.5f),
                    GetColorU32(text_color), label, label_end);

        return pressed;
    }

    bool CheckboxClicked(const char* label, bool* v)
    {
        ImGuiWindow* window = GetCurrentWindow();

        if (window->SkipItems) return false;

        ImGuiContext& g = *GImGui;
        const ImGuiStyle& style = g.Style;
        const ImGuiID id = window->GetID(label);
        const ImVec2 label_size = CalcTextSize(label, NULL, true), pos = window->DC.CursorPos;
        const ImRect bb(pos + ImVec2(GetContentRegionMax().x - 70 * c::scale, 0), pos + ImVec2(GetContentRegionMax().x - 50 * c::scale, 32 * c::scale));

        if (!ItemAdd(bb, id)) return false;

        bool hovered, held, pressed = ButtonBehavior(bb, id, &hovered, &held, NULL);
        
        custom::Checkbox(label, v);

        PushFont(font::icomoon_page);
        GetWindowDrawList()->AddText(pos + ImVec2(GetContentRegionMax().x - 65 * c::scale, 7 * c::scale), GetColorU32(c::text::text), "l");
        PopFont();

        return pressed;
    }

    static float CalcMaxPopupHeightFromItemCount(int items_count)
    {
        ImGuiContext& g = *GImGui;
        if (items_count <= 0)
            return FLT_MAX;
        return (g.FontSize + g.Style.ItemSpacing.y) * items_count - g.Style.ItemSpacing.y + (g.Style.WindowPadding.y * 2);
    }

    int rotation_start_index;
    void ImRotateStart()
    {
        rotation_start_index = ImGui::GetWindowDrawList()->VtxBuffer.Size;
    }

    ImVec2 ImRotationCenter()
    {
        ImVec2 l(FLT_MAX, FLT_MAX), u(-FLT_MAX, -FLT_MAX);

        const auto& buf = ImGui::GetWindowDrawList()->VtxBuffer;
        for (int i = rotation_start_index; i < buf.Size; i++)
            l = ImMin(l, buf[i].pos), u = ImMax(u, buf[i].pos);

        return ImVec2((l.x + u.x) / 2, (l.y + u.y) / 2);
    }

    void ImRotateEnd(float rad, ImVec2 center = ImRotationCenter())
    {
        float s = sin(rad), c = cos(rad);
        center = ImRotate(center, s, c) - center;

        auto& buf = ImGui::GetWindowDrawList()->VtxBuffer;
        for (int i = rotation_start_index; i < buf.Size; i++)
            buf[i].pos = ImRotate(buf[i].pos, s, c) - center;
    }

    struct begin_state
    {
        ImVec4 background, text, outline;
        float combo_size = 0.f;
        float arrow_roll = 0.0f;
        float popup_alpha = 0.0f;
        float popup_offset = 0.0f;
        float field_emphasis = 0.0f;
        bool opened_combo = false;
        bool hovered = false;
        bool want_open = false;
    };

    bool BeginCombo(const char* label, const char* preview_value, int val, bool multi, ImGuiComboFlags flags)
    {
        ImGuiContext& g = *GImGui;
        ImGuiWindow* window = GetCurrentWindow();

        g.NextWindowData.ClearFlags();
        if (window->SkipItems) return false;

        const ImGuiID id = window->GetID(label);
        const ImGuiStyle& style = g.Style;
        const char* label_end = FindRenderedTextEnd(label);
        const ImVec2 label_size = CalcTextSize(label, label_end, true);
        const float width = GetContentRegionMax().x - style.WindowPadding.x;
        const float control_scale = c::scale * c::widget_scale;
        const float field_height = 24.0f * control_scale;
        const float gap = 5.0f * control_scale;

        const ImRect total_bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(width, label_size.y + gap + field_height));
        const ImRect field_bb(total_bb.Min + ImVec2(0.0f, label_size.y + gap), total_bb.Max);
        ItemSize(total_bb, 0.0f);

        if (!ItemAdd(total_bb, id, &field_bb)) return false;

        bool hovered, held, pressed = ButtonBehavior(field_bb, id, &hovered, &held);
        const ImGuiID popup_id = ImHashStr("##CustomComboPopup", 0, id);

        static std::map<ImGuiID, begin_state> anim;
        auto it_anim = anim.find(id);

        if (it_anim == anim.end())
        {
            anim.insert({ id, begin_state() });
            it_anim = anim.find(id);
            it_anim->second.want_open = false;
        }

        bool popup_open = IsPopupOpen(popup_id, ImGuiPopupFlags_None);
        const bool popup_was_open = it_anim->second.opened_combo;
        if (!popup_open && popup_was_open && !pressed)
            it_anim->second.want_open = false;
        if (pressed) {
            it_anim->second.want_open = !(popup_open || it_anim->second.want_open);
            if (it_anim->second.want_open && !popup_open)
                OpenPopupEx(popup_id, ImGuiPopupFlags_None);
        }
        popup_open = IsPopupOpen(popup_id, ImGuiPopupFlags_None);
        const float popup_target = (popup_open && it_anim->second.want_open)
            ? ImClamp(CalcMaxPopupHeightFromItemCount(val) + 6.0f * c::scale, 0.0f, 220.0f * c::scale)
            : 0.0f;
        it_anim->second.arrow_roll = ImLerp(it_anim->second.arrow_roll, (popup_open && it_anim->second.want_open) ? 1.0f : 0.0f, g.IO.DeltaTime * 12.0f);
        it_anim->second.text = ImLerp(it_anim->second.text, (popup_open && it_anim->second.want_open) ? c::text::text_active : hovered ? c::text::text_hov : c::text::text_active, g.IO.DeltaTime * 10.0f);
        const ImVec4 target_combo_bg = hovered || popup_open ? ImVec4(0.0f, 0.0f, 0.0f, 0.92f) : ImVec4(0.0f, 0.0f, 0.0f, 0.84f);
        it_anim->second.background = ImLerp(it_anim->second.background, target_combo_bg, g.IO.DeltaTime * 10.0f);
        it_anim->second.combo_size = ImLerp(it_anim->second.combo_size, popup_target, g.IO.DeltaTime * 14.0f);
        it_anim->second.popup_alpha = ImLerp(it_anim->second.popup_alpha, (popup_open && it_anim->second.want_open) ? 1.0f : 0.0f, g.IO.DeltaTime * 14.0f);
        it_anim->second.popup_offset = ImLerp(it_anim->second.popup_offset, (popup_open && it_anim->second.want_open) ? 0.0f : -6.0f * c::scale, g.IO.DeltaTime * 14.0f);
        it_anim->second.field_emphasis = ImLerp(it_anim->second.field_emphasis, hovered || popup_open ? 1.0f : 0.0f, g.IO.DeltaTime * 12.0f);

        if (popup_open && !it_anim->second.want_open && it_anim->second.combo_size <= 2.0f && it_anim->second.popup_alpha <= 0.04f)
        {
            ClosePopupToLevel(g.BeginPopupStack.Size, true);
            popup_open = IsPopupOpen(popup_id, ImGuiPopupFlags_None);
        }

        it_anim->second.opened_combo = popup_open;

        const float field_rounding = 6.0f * control_scale;
        GetWindowDrawList()->AddText(total_bb.Min, GetColorU32(WithAlpha(c::text::text, 0.5f)), label, label_end);
        GetWindowDrawList()->AddRectFilled(field_bb.Min, field_bb.Max, GetColorU32(it_anim->second.background), field_rounding);
        if (it_anim->second.field_emphasis > 0.01f)
        {
            GetWindowDrawList()->AddRectFilled(
                field_bb.Min - ImVec2(1.0f * control_scale, 1.0f * control_scale),
                field_bb.Max + ImVec2(1.0f * control_scale, 1.0f * control_scale),
                GetColorU32(AccentShade(0.18f), 0.06f * it_anim->second.field_emphasis),
                field_rounding + 1.0f * control_scale
            );
        }
        GetWindowDrawList()->AddRectFilledMultiColor(
            field_bb.Min + ImVec2(1.0f * control_scale, 1.0f * control_scale),
            ImVec2(field_bb.Max.x - 1.0f * control_scale, field_bb.Min.y + field_height * 0.46f),
            GetColorU32(WithAlpha(c::text::text_active, 0.05f)),
            GetColorU32(WithAlpha(c::text::text_active, 0.02f)),
            GetColorU32(WithAlpha(c::text::text_active, 0.00f)),
            GetColorU32(WithAlpha(c::text::text_active, 0.00f))
        );
        GetWindowDrawList()->AddRect(
            field_bb.Min,
            field_bb.Max,
            GetColorU32(popup_open ? AccentLift(0.08f) : c::widget::outlinecolor, hovered || popup_open ? 1.0f : 0.78f),
            field_rounding,
            0,
            1.0f * control_scale
        );

        PushClipRect(field_bb.Min, field_bb.Max, true);
        GetWindowDrawList()->AddText(
            field_bb.Min + ImVec2(10.0f * control_scale, (field_height - CalcTextSize(preview_value ? preview_value : "None").y) * 0.5f),
            GetColorU32(it_anim->second.text),
            preview_value ? preview_value : "None"
        );
        PopClipRect();

        const float arrow_pad = 14.0f * control_scale;
        const ImVec2 arrow_center(field_bb.Max.x - arrow_pad, field_bb.Min.y + field_height * 0.5f);
        const float arrow_size = 3.6f * control_scale;
        const float arrow_dir = it_anim->second.arrow_roll;
        const float arrow_offset = ImLerp(-1.2f * control_scale, 1.2f * control_scale, arrow_dir);
        GetWindowDrawList()->AddLine(
            ImVec2(arrow_center.x - arrow_size, arrow_center.y - arrow_offset),
            ImVec2(arrow_center.x, arrow_center.y + arrow_size - arrow_offset),
            GetColorU32(c::text::text_active),
            1.4f * control_scale
        );
        GetWindowDrawList()->AddLine(
            ImVec2(arrow_center.x, arrow_center.y + arrow_size - arrow_offset),
            ImVec2(arrow_center.x + arrow_size, arrow_center.y - arrow_offset),
            GetColorU32(c::text::text_active),
            1.4f * control_scale
        );

        if (!IsRectVisible(total_bb.Min, total_bb.Max + ImVec2(0, 2 * control_scale)))
        {
            it_anim->second.want_open = false;
            if (popup_open && it_anim->second.combo_size <= 2.0f)
                ClosePopupToLevel(g.BeginPopupStack.Size, true);
            it_anim->second.opened_combo = false;
            it_anim->second.combo_size = 0.f;
            it_anim->second.popup_alpha = 0.0f;
        }

        if (!popup_open && it_anim->second.combo_size < 2.f && it_anim->second.popup_alpha < 0.04f) return false;

        ImGui::SetNextWindowPos(ImVec2(field_bb.Min.x, field_bb.Max.y + 5.0f * control_scale + it_anim->second.popup_offset));
        ImGui::SetNextWindowSize(ImVec2(field_bb.GetWidth(), ImMax(1.0f, it_anim->second.combo_size)));
        ImGui::SetNextWindowBgAlpha(0.90f * ImMax(0.0f, it_anim->second.popup_alpha));
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

        PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.0f, 0.0f, 0.0f, 0.90f));
        PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.90f));
        PushStyleColor(ImGuiCol_Border, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
        PushStyleVar(ImGuiStyleVar_WindowRounding, 5.0f * control_scale);
        PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6.0f * control_scale, 6.0f * control_scale));
        PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

        bool ret = BeginPopupEx(popup_id, window_flags);

        PopStyleVar(3);
        PopStyleColor(3);

        if (!ret)
        {
            it_anim->second.hovered = false;
            it_anim->second.opened_combo = false;
            return false;
        }

        it_anim->second.hovered = IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem | ImGuiHoveredFlags_ChildWindows);
        it_anim->second.opened_combo = true;

        return true;
    }

    void EndCombo()
    {
        EndPopup();
    }

    void MultiCombo(const char* label, bool variable[], const char* labels[], int count)
    {
        ImGuiContext& g = *GImGui;

        std::string preview = "None";

        for (auto i = 0, j = 0; i < count; i++)
        {
            if (variable[i])
            {
                if (j)
                    preview += (", ") + (std::string)labels[i];
                else
                    preview = labels[i];

                j++;
            }
        }

        if (BeginCombo(label, preview.c_str(), count))
        {
            for (auto i = 0; i < count; i++)
            {
                PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f * c::scale, 8.0f * c::scale));
                PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f * c::scale, 8.0f * c::scale));
                custom::Selectable(labels[i], &variable[i], ImGuiSelectableFlags_DontClosePopups);
                PopStyleVar(2);
            }
            EndCombo();
        }

        preview = ("None");
    }

    bool BeginComboPreview()
    {
        ImGuiContext& g = *GImGui;
        ImGuiWindow* window = g.CurrentWindow;
        ImGuiComboPreviewData* preview_data = &g.ComboPreviewData;

        if (window->SkipItems || !(g.LastItemData.StatusFlags & ImGuiItemStatusFlags_Visible)) return false;

        IM_ASSERT(g.LastItemData.Rect.Min.x == preview_data->PreviewRect.Min.x && g.LastItemData.Rect.Min.y == preview_data->PreviewRect.Min.y);

        if (!window->ClipRect.Overlaps(preview_data->PreviewRect)) return false;

        preview_data->BackupCursorPos = window->DC.CursorPos;
        preview_data->BackupCursorMaxPos = window->DC.CursorMaxPos;
        preview_data->BackupCursorPosPrevLine = window->DC.CursorPosPrevLine;
        preview_data->BackupPrevLineTextBaseOffset = window->DC.PrevLineTextBaseOffset;
        preview_data->BackupLayout = window->DC.LayoutType;
        window->DC.CursorPos = preview_data->PreviewRect.Min + g.Style.FramePadding;
        window->DC.CursorMaxPos = window->DC.CursorPos;
        window->DC.LayoutType = ImGuiLayoutType_Horizontal;
        window->DC.IsSameLine = false;
        PushClipRect(preview_data->PreviewRect.Min, preview_data->PreviewRect.Max, true);

        return true;
    }

    void EndComboPreview()
    {
        ImGuiContext& g = *GImGui;
        ImGuiWindow* window = g.CurrentWindow;
        ImGuiComboPreviewData* preview_data = &g.ComboPreviewData;

        ImDrawList* draw_list = window->DrawList;
        if (window->DC.CursorMaxPos.x < preview_data->PreviewRect.Max.x && window->DC.CursorMaxPos.y < preview_data->PreviewRect.Max.y)
            if (draw_list->CmdBuffer.Size > 1)
            {
                draw_list->_CmdHeader.ClipRect = draw_list->CmdBuffer[draw_list->CmdBuffer.Size - 1].ClipRect = draw_list->CmdBuffer[draw_list->CmdBuffer.Size - 2].ClipRect;
                draw_list->_TryMergeDrawCmds();
            }
        PopClipRect();
        window->DC.CursorPos = preview_data->BackupCursorPos;
        window->DC.CursorMaxPos = ImMax(window->DC.CursorMaxPos, preview_data->BackupCursorMaxPos);
        window->DC.CursorPosPrevLine = preview_data->BackupCursorPosPrevLine;
        window->DC.PrevLineTextBaseOffset = preview_data->BackupPrevLineTextBaseOffset;
        window->DC.LayoutType = preview_data->BackupLayout;
        window->DC.IsSameLine = false;
        preview_data->PreviewRect = ImRect();
    }

    static const char* Items_ArrayGetter(void* data, int idx)
    {
        const char* const* items = (const char* const*)data;
        return items[idx];
    }

    static const char* Items_SingleStringGetter(void* data, int idx)
    {
        const char* items_separated_by_zeros = (const char*)data;
        int items_count = 0;
        const char* p = items_separated_by_zeros;
        while (*p)
        {
            if (idx == items_count)
                break;
            p += strlen(p) + 1;
            items_count++;
        }
        return *p ? p : NULL;
    }

    bool Combo(const char* label, int* current_item, const char* (*getter)(void* user_data, int idx), void* user_data, int items_count, int popup_max_height_in_items)
    {
        ImGuiContext& g = *GImGui;

        const char* preview_value = NULL;
        if (*current_item >= 0 && *current_item < items_count)
            preview_value = getter(user_data, *current_item);

        if (popup_max_height_in_items != -1 && !(g.NextWindowData.Flags & ImGuiNextWindowDataFlags_HasSizeConstraint))
            SetNextWindowSizeConstraints(ImVec2(0, 0), ImVec2(FLT_MAX, CalcMaxPopupHeightFromItemCount(popup_max_height_in_items) * c::scale));

        if (!BeginCombo(label, preview_value, items_count, ImGuiComboFlags_None)) return false;

        bool value_changed = false;
        PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f * c::scale, 8.0f * c::scale));
        for (int i = 0; i < items_count; i++)
        {
            const char* item_text = getter(user_data, i);
            if (item_text == NULL)
                item_text = "*Unknown item*";

            PushID(i);
            const bool item_selected = (i == *current_item);
            if (custom::Selectable(item_text, item_selected) && *current_item != i)
            {
                value_changed = true;
                *current_item = i;
            }
            if (item_selected)
                SetItemDefaultFocus();
            PopID();
        }
        PopStyleVar();

        EndCombo();

        if (value_changed)
            MarkItemEdited(g.LastItemData.ID);

        return value_changed;
    }

    bool Combo(const char* label, int* current_item, const char* const items[], int items_count, int height_in_items)
    {
        const bool value_changed = Combo(label, current_item, Items_ArrayGetter, (void*)items, items_count, height_in_items);
        return value_changed;
    }

    bool Combo(const char* label, int* current_item, const char* items_separated_by_zeros, int height_in_items)
    {
        int items_count = 0;
        const char* p = items_separated_by_zeros;
        while (*p)
        {
            p += strlen(p) + 1;
            items_count++;
        }
        bool value_changed = Combo(label, current_item, Items_SingleStringGetter, (void*)items_separated_by_zeros, items_count, height_in_items);
        return value_changed;
    }

    struct select_state
    {
        ImVec4 text;
    };

    bool Selectable(const char* label, bool selected, ImGuiSelectableFlags flags, const ImVec2& size_arg)
    {
        ImGuiWindow* window = GetCurrentWindow();
        if (window->SkipItems)
            return false;

        ImGuiContext& g = *GImGui;
        const ImGuiStyle& style = g.Style;

        ImGuiID id = window->GetID(label);
        ImVec2 label_size = CalcTextSize(label, NULL, true);
        ImVec2 size(size_arg.x != 0.0f ? size_arg.x : label_size.x, size_arg.y != 0.0f ? size_arg.y : label_size.y);
        ImVec2 pos = window->DC.CursorPos;
        pos.y += window->DC.CurrLineTextBaseOffset;
        ItemSize(size, 0.0f);

        const bool span_all_columns = (flags & ImGuiSelectableFlags_SpanAllColumns) != 0;
        const float min_x = span_all_columns ? window->ParentWorkRect.Min.x : pos.x;
        const float max_x = span_all_columns ? window->ParentWorkRect.Max.x : window->WorkRect.Max.x;
        if (size_arg.x == 0.0f || (flags & ImGuiSelectableFlags_SpanAvailWidth)) size.x = ImMax(label_size.x, max_x - min_x);

        const ImVec2 text_min = pos;
        const ImVec2 text_max(min_x + size.x, pos.y + size.y);

        ImRect bb(min_x, pos.y, text_max.x, text_max.y);
        if ((flags & ImGuiSelectableFlags_NoPadWithHalfSpacing) == 0)
        {
            const float spacing_x = span_all_columns ? 0.0f : style.ItemSpacing.x;
            const float spacing_y = style.ItemSpacing.y;
            const float spacing_L = IM_TRUNC(spacing_x * 0.50f);
            const float spacing_U = IM_TRUNC(spacing_y * 0.50f);
            bb.Min.x -= spacing_L;
            bb.Min.y -= spacing_U;
            bb.Max.x += (spacing_x - spacing_L);
            bb.Max.y += (spacing_y - spacing_U);
        }

        const float backup_clip_rect_min_x = window->ClipRect.Min.x;
        const float backup_clip_rect_max_x = window->ClipRect.Max.x;
        if (span_all_columns)
        {
            window->ClipRect.Min.x = window->ParentWorkRect.Min.x;
            window->ClipRect.Max.x = window->ParentWorkRect.Max.x;
        }

        const bool disabled_item = (flags & ImGuiSelectableFlags_Disabled) != 0;
        const bool item_add = ItemAdd(bb, id, NULL, disabled_item ? ImGuiItemFlags_Disabled : ImGuiItemFlags_None);
        if (span_all_columns)
        {
            window->ClipRect.Min.x = backup_clip_rect_min_x;
            window->ClipRect.Max.x = backup_clip_rect_max_x;
        }

        if (!item_add) return false;

        const bool disabled_global = (g.CurrentItemFlags & ImGuiItemFlags_Disabled) != 0;
        if (disabled_item && !disabled_global) BeginDisabled();

        if (span_all_columns && window->DC.CurrentColumns) PushColumnsBackground();
        else if (span_all_columns && g.CurrentTable) TablePushBackgroundChannel();

        ImGuiButtonFlags button_flags = 0;
        if (flags & ImGuiSelectableFlags_NoHoldingActiveID) { button_flags |= ImGuiButtonFlags_NoHoldingActiveId; }
        if (flags & ImGuiSelectableFlags_NoSetKeyOwner) { button_flags |= ImGuiButtonFlags_NoSetKeyOwner; }
        if (flags & ImGuiSelectableFlags_SelectOnClick) { button_flags |= ImGuiButtonFlags_PressedOnClick; }
        if (flags & ImGuiSelectableFlags_SelectOnRelease) { button_flags |= ImGuiButtonFlags_PressedOnRelease; }
        if (flags & ImGuiSelectableFlags_AllowDoubleClick) { button_flags |= ImGuiButtonFlags_PressedOnClickRelease | ImGuiButtonFlags_PressedOnDoubleClick; }
        if ((flags & ImGuiSelectableFlags_AllowOverlap) || (g.LastItemData.InFlags & ImGuiItemFlags_AllowOverlap)) { button_flags |= ImGuiButtonFlags_AllowOverlap; }

        const bool was_selected = selected;
        bool hovered, held, pressed = ButtonBehavior(bb, id, &hovered, &held, button_flags);

        if ((flags & ImGuiSelectableFlags_SelectOnNav) && g.NavJustMovedToId != 0 && g.NavJustMovedToFocusScopeId == g.CurrentFocusScopeId)
            if (g.NavJustMovedToId == id)  selected = pressed = true;

        if (pressed || (hovered && (flags & ImGuiSelectableFlags_SetNavIdOnHover)))
        {
            if (!g.NavDisableMouseHover && g.NavWindow == window && g.NavLayer == window->DC.NavLayerCurrent)
            {
                SetNavID(id, window->DC.NavLayerCurrent, g.CurrentFocusScopeId, WindowRectAbsToRel(window, bb));
                g.NavDisableHighlight = true;
            }
        }
        if (pressed) MarkItemEdited(id);

        if (selected != was_selected)  g.LastItemData.StatusFlags |= ImGuiItemStatusFlags_ToggledSelection;

        if (g.NavId == id) RenderNavHighlight(bb, id, ImGuiNavHighlightFlags_TypeThin | ImGuiNavHighlightFlags_NoRounding);

        if (span_all_columns && window->DC.CurrentColumns) PopColumnsBackground();
        else if (span_all_columns && g.CurrentTable) TablePopBackgroundChannel();

        static std::map<ImGuiID, select_state> anim;
        auto it_anim = anim.find(id);

        if (it_anim == anim.end())
        {
            anim.insert({ id, select_state() });
            it_anim = anim.find(id);
        }

        const char* label_end = FindRenderedTextEnd(label);
        it_anim->second.text = ImLerp(it_anim->second.text, selected ? c::text::text_active : hovered ? c::text::text_hov : c::text::text, g.IO.DeltaTime * 8.f);

        const bool popup_selectable = (window->Flags & ImGuiWindowFlags_Popup) != 0;
        const ImVec4 selectable_fill = popup_selectable
            ? (selected ? AccentShade(0.76f, 0.26f) : hovered ? ImVec4(0.0f, 0.0f, 0.0f, 0.76f) : ImVec4(0.0f, 0.0f, 0.0f, 0.48f))
            : (selected ? AccentShade(0.18f, 0.30f) : hovered ? ImVec4(0.086f, 0.086f, 0.086f, 0.72f) : ImVec4(0.055f, 0.055f, 0.055f, 0.42f));

        GetWindowDrawList()->AddRectFilled(
            bb.Min,
            bb.Max,
            GetColorU32(selectable_fill),
            6.0f * c::scale
        );
        GetWindowDrawList()->AddRect(
            bb.Min,
            bb.Max,
            GetColorU32(selected ? AccentLift(0.08f) : c::widget::outlinecolor, selected ? 0.88f : 0.45f),
            6.0f * c::scale,
            0,
            1.0f * c::scale
        );
        if (selected) {
            const float mark = 7.0f * c::scale;
            const ImVec2 mark_min(bb.Max.x - (mark + 10.0f * c::scale), bb.Min.y + (bb.GetHeight() - mark) * 0.5f);
            GetWindowDrawList()->AddCircleFilled(mark_min + ImVec2(mark * 0.5f, mark * 0.5f), mark * 0.5f, GetColorU32(c::accent), 18);
        }

        PushStyleColor(ImGuiCol_Text, GetColorU32(it_anim->second.text));
        RenderTextClipped(text_min + ImVec2(4.0f * c::scale, 0.0f), text_max - ImVec2(18.0f * c::scale, 0.0f), label, label_end, &label_size, style.SelectableTextAlign, &bb);
        PopStyleColor();

        if (pressed && (window->Flags & ImGuiWindowFlags_Popup) && !(flags & ImGuiSelectableFlags_DontClosePopups) && !(g.LastItemData.InFlags & ImGuiItemFlags_SelectableDontClosePopup)) CloseCurrentPopup();

        if (disabled_item && !disabled_global) EndDisabled();

        return pressed;
    }

    bool Selectable(const char* label, bool* p_selected, ImGuiSelectableFlags flags, const ImVec2& size_arg)
    {
        if (Selectable(label, *p_selected, flags, size_arg))
        {
            *p_selected = !*p_selected;
            return true;
        }
        return false;
    }

    static void ColorEditRestoreH(const float* col, float* H)
    {
        ImGuiContext& g = *GImGui;
        IM_ASSERT(g.ColorEditCurrentID != 0);
        if (g.ColorEditSavedID != g.ColorEditCurrentID || g.ColorEditSavedColor != ImGui::ColorConvertFloat4ToU32(ImVec4(col[0], col[1], col[2], 0)))
            return;
        *H = g.ColorEditSavedHue;
    }

    static void ColorEditRestoreHS(const float* col, float* H, float* S, float* V)
    {
        ImGuiContext& g = *GImGui;
        IM_ASSERT(g.ColorEditCurrentID != 0);
        if (g.ColorEditSavedID != g.ColorEditCurrentID || g.ColorEditSavedColor != ImGui::ColorConvertFloat4ToU32(ImVec4(col[0], col[1], col[2], 0))) return;

        if (*S == 0.0f || (*H == 0.0f && g.ColorEditSavedHue == 1))
            *H = g.ColorEditSavedHue;

        if (*V == 0.0f) *S = g.ColorEditSavedSat;
    }

    struct edit_state
    {
        ImVec4 text;
    };

    bool ColorEdit4(const char* label, float col[4], ImGuiColorEditFlags flags)
    {
        ImGuiWindow* window = GetCurrentWindow();
        if (window->SkipItems) return false;

        ImGuiContext& g = *GImGui;
        const ImGuiStyle& style = g.Style;
        const float square_sz = GetFrameHeight() * c::scale;
        const float w_full = CalcItemWidth();
        const float w_button = (flags & ImGuiColorEditFlags_NoSmallPreview) ? 0.0f : (square_sz + style.ItemInnerSpacing.x * c::scale);
        const float w_inputs = w_full - w_button;
        const char* label_display_end = FindRenderedTextEnd(label);
        g.NextItemData.ClearFlags();

        BeginGroup();
        PushID(label);
        const bool set_current_color_edit_id = (g.ColorEditCurrentID == 0);
        if (set_current_color_edit_id)
            g.ColorEditCurrentID = window->IDStack.back();

        const ImGuiColorEditFlags flags_untouched = flags;
        if (flags & ImGuiColorEditFlags_NoInputs)
            flags = (flags & (~ImGuiColorEditFlags_DisplayMask_)) | ImGuiColorEditFlags_DisplayRGB | ImGuiColorEditFlags_NoOptions;

        if (!(flags & ImGuiColorEditFlags_NoOptions))
            ColorEditOptionsPopup(col, flags);

        if (!(flags & ImGuiColorEditFlags_DisplayMask_))
            flags |= (g.ColorEditOptions & ImGuiColorEditFlags_DisplayMask_);
        if (!(flags & ImGuiColorEditFlags_DataTypeMask_))
            flags |= (g.ColorEditOptions & ImGuiColorEditFlags_DataTypeMask_);
        if (!(flags & ImGuiColorEditFlags_PickerMask_))
            flags |= (g.ColorEditOptions & ImGuiColorEditFlags_PickerMask_);
        if (!(flags & ImGuiColorEditFlags_InputMask_))
            flags |= (g.ColorEditOptions & ImGuiColorEditFlags_InputMask_);
        flags |= (g.ColorEditOptions & ~(ImGuiColorEditFlags_DisplayMask_ | ImGuiColorEditFlags_DataTypeMask_ | ImGuiColorEditFlags_PickerMask_ | ImGuiColorEditFlags_InputMask_));
        IM_ASSERT(ImIsPowerOfTwo(flags & ImGuiColorEditFlags_DisplayMask_));
        IM_ASSERT(ImIsPowerOfTwo(flags & ImGuiColorEditFlags_InputMask_));

        const bool alpha = (flags & ImGuiColorEditFlags_NoAlpha) == 0;
        const bool hdr = (flags & ImGuiColorEditFlags_HDR) != 0;
        const int components = alpha ? 4 : 3;

        float f[4] = { col[0], col[1], col[2], alpha ? col[3] : 1.0f };
        if ((flags & ImGuiColorEditFlags_InputHSV) && (flags & ImGuiColorEditFlags_DisplayRGB))
            ColorConvertHSVtoRGB(f[0], f[1], f[2], f[0], f[1], f[2]);
        else if ((flags & ImGuiColorEditFlags_InputRGB) && (flags & ImGuiColorEditFlags_DisplayHSV))
        {
            ColorConvertRGBtoHSV(f[0], f[1], f[2], f[0], f[1], f[2]);
            ColorEditRestoreHS(col, &f[0], &f[1], &f[2]);
        }
        int i[4] = { IM_F32_TO_INT8_UNBOUND(f[0]), IM_F32_TO_INT8_UNBOUND(f[1]), IM_F32_TO_INT8_UNBOUND(f[2]), IM_F32_TO_INT8_UNBOUND(f[3]) };

        bool value_changed = false;
        bool value_changed_as_float = false;

        const ImVec2 pos = window->DC.CursorPos;
        const float inputs_offset_x = (style.ColorButtonPosition == ImGuiDir_Left) ? w_button : 0.0f;
        window->DC.CursorPos.x = pos.x + inputs_offset_x;

        if ((flags & (ImGuiColorEditFlags_DisplayRGB | ImGuiColorEditFlags_DisplayHSV)) != 0 && (flags & ImGuiColorEditFlags_NoInputs) == 0)
        {
            const float w_item_one = ImMax(1.0f, IM_FLOOR((w_inputs - (style.ItemInnerSpacing.x * c::scale) * (components - 1)) / (float)components));
            const float w_item_last = ImMax(1.0f, IM_FLOOR(w_inputs - (w_item_one + style.ItemInnerSpacing.x * c::scale) * (components - 1)));

            const bool hide_prefix = (w_item_one <= CalcTextSize((flags & ImGuiColorEditFlags_Float) ? "M:0.000" : "M:000").x);
            static const char* ids[4] = { "##X", "##Y", "##Z", "##W" };
            static const char* fmt_table_int[3][4] =
            {
                {   "%3d",   "%3d",   "%3d",   "%3d" },
                { "R:%3d", "G:%3d", "B:%3d", "A:%3d" },
                { "H:%3d", "S:%3d", "V:%3d", "A:%3d" }
            };
            static const char* fmt_table_float[3][4] =
            {
                {   "%0.3f",   "%0.3f",   "%0.3f",   "%0.3f" },
                { "R:%0.3f", "G:%0.3f", "B:%0.3f", "A:%0.3f" },
                { "H:%0.3f", "S:%0.3f", "V:%0.3f", "A:%0.3f" }
            };
            const int fmt_idx = hide_prefix ? 0 : (flags & ImGuiColorEditFlags_DisplayHSV) ? 2 : 1;

            for (int n = 0; n < components; n++)
            {
                if (n > 0)
                    SameLine(0, style.ItemInnerSpacing.x * c::scale);
                SetNextItemWidth((n + 1 < components) ? w_item_one : w_item_last);

                if (flags & ImGuiColorEditFlags_Float)
                {
                    value_changed |= DragFloat(ids[n], &f[n], 1.0f / 255.0f, 0.0f, hdr ? 0.0f : 1.0f, fmt_table_float[fmt_idx][n]);
                    value_changed_as_float |= value_changed;
                }
                else
                {
                    value_changed |= DragInt(ids[n], &i[n], 1.0f, 0, hdr ? 0 : 255, fmt_table_int[fmt_idx][n]);
                }
                if (!(flags & ImGuiColorEditFlags_NoOptions))
                    OpenPopupOnItemClick("context", ImGuiPopupFlags_MouseButtonRight);
            }
        }
        else if ((flags & ImGuiColorEditFlags_DisplayHex) != 0 && (flags & ImGuiColorEditFlags_NoInputs) == 0)
        {
            if (!(flags & ImGuiColorEditFlags_NoOptions))
                OpenPopupOnItemClick("context", ImGuiPopupFlags_MouseButtonRight);
        }

        char buf[64];
        ImFormatString(buf, IM_ARRAYSIZE(buf), "#%02X%02X%02X", ImClamp(i[0], 0, 255), ImClamp(i[1], 0, 255), ImClamp(i[2], 0, 255));
        if ((flags & ImGuiColorEditFlags_NoInputs) == 0)
        {
            SetNextItemWidth(w_inputs);
            if (InputText("", buf, IM_ARRAYSIZE(buf), ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase))
            {
                value_changed = true;
                char* p = buf;
                while (*p == '#' || ImCharIsBlankA(*p))
                    p++;
                i[0] = i[1] = i[2] = 0;
                i[3] = 0xFF;
                int r;
                r = sscanf(p, "%02X%02X%02X", (unsigned int*)&i[0], (unsigned int*)&i[1], (unsigned int*)&i[2]);
            }
        }

        ImGuiWindow* picker_active_window = NULL;
        if (!(flags & ImGuiColorEditFlags_NoSmallPreview))
        {
            const float button_offset_x = ((flags & ImGuiColorEditFlags_NoInputs) || (style.ColorButtonPosition == ImGuiDir_Left)) ? 0.0f : w_inputs + style.ItemInnerSpacing.x * c::scale;
            window->DC.CursorPos = ImVec2(pos.x + button_offset_x, pos.y);

            const ImVec4 col_v4(col[0], col[1], col[2], alpha ? col[3] : 1.0f);
            if (ColorButton("##ColorButton", col_v4, flags))
            {
                if (!(flags & ImGuiColorEditFlags_NoPicker))
                {
                    g.ColorPickerRef = col_v4;
                    OpenPopup("picker");
                    SetNextWindowPos(g.LastItemData.Rect.GetBL() + ImVec2(0.0f, style.ItemSpacing.y * c::scale));
                }
            }
            if (!(flags & ImGuiColorEditFlags_NoOptions))
                OpenPopupOnItemClick("context", ImGuiPopupFlags_MouseButtonRight);

            PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.0f, 0.0f, 0.0f, 0.94f));
            PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.94f));
            PushStyleVar(ImGuiStyleVar_PopupRounding, 8.0f * c::scale);
            PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f * c::scale, 10.0f * c::scale));
            PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f * c::scale, 8.0f * c::scale));
            if (BeginPopup("picker"))
            {
                if (g.CurrentWindow->BeginCount == 1)
                {
                    picker_active_window = g.CurrentWindow;

                    ImGuiColorEditFlags picker_flags_to_forward = ImGuiColorEditFlags_DataTypeMask_ | ImGuiColorEditFlags_PickerMask_ | ImGuiColorEditFlags_InputMask_ | ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_AlphaBar;
                    ImGuiColorEditFlags picker_flags = (flags_untouched & picker_flags_to_forward) | ImGuiColorEditFlags_DisplayMask_ | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_AlphaPreviewHalf;
                    SetNextItemWidth(square_sz * 15.0f);
                    value_changed |= ColorPicker4("##picker", col, picker_flags, &g.ColorPickerRef.x);
                }
                EndPopup();
            }
            PopStyleColor(2);
            PopStyleVar(3);
        }

        if (label != label_display_end && !(flags & ImGuiColorEditFlags_NoLabel))
        {
            static std::map<ImGuiID, edit_state> anim;
            auto it_anim = anim.find(ImGui::GetID(label));

            if (it_anim == anim.end())
            {
                anim.insert({ ImGui::GetID(label), edit_state() });
                it_anim = anim.find(ImGui::GetID(label));
            }

            SameLine(0.0f, style.ItemInnerSpacing.x * c::scale);
            window->DC.CursorPos.x = pos.x - w_button + ((flags & ImGuiColorEditFlags_NoInputs) ? w_button : w_full);

            GetWindowDrawList()->AddText(window->DC.CursorPos + ImVec2(0, (32 * c::scale - CalcTextSize(buf).y) / 2), GetColorU32(c::text::text), label);
        }

        if (value_changed && picker_active_window == NULL)
        {
            if (!value_changed_as_float)
                for (int n = 0; n < 4; n++)
                    f[n] = i[n] / 255.0f;
            if ((flags & ImGuiColorEditFlags_DisplayHSV) && (flags & ImGuiColorEditFlags_InputRGB))
            {
                g.ColorEditSavedHue = f[0];
                g.ColorEditSavedSat = f[1];
                ColorConvertHSVtoRGB(f[0], f[1], f[2], f[0], f[1], f[2]);
                g.ColorEditSavedID = g.ColorEditCurrentID;
                g.ColorEditSavedColor = ColorConvertFloat4ToU32(ImVec4(f[0], f[1], f[2], 0));
            }
            if ((flags & ImGuiColorEditFlags_DisplayRGB) && (flags & ImGuiColorEditFlags_InputHSV))
                ColorConvertRGBtoHSV(f[0], f[1], f[2], f[0], f[1], f[2]);

            col[0] = f[0];
            col[1] = f[1];
            col[2] = f[2];
            if (alpha)
                col[3] = f[3];
        }

        if (set_current_color_edit_id)
            g.ColorEditCurrentID = 0;
        PopID();
        EndGroup();

        if ((g.LastItemData.StatusFlags & ImGuiItemStatusFlags_HoveredRect) && !(flags & ImGuiColorEditFlags_NoDragDrop) && BeginDragDropTarget())
        {
            bool accepted_drag_drop = false;
            if (const ImGuiPayload* payload = AcceptDragDropPayload(IMGUI_PAYLOAD_TYPE_COLOR_3F))
            {
                memcpy((float*)col, payload->Data, sizeof(float) * 3);
                value_changed = accepted_drag_drop = true;
            }
            if (const ImGuiPayload* payload = AcceptDragDropPayload(IMGUI_PAYLOAD_TYPE_COLOR_4F))
            {
                memcpy((float*)col, payload->Data, sizeof(float) * components);
                value_changed = accepted_drag_drop = true;
            }

            if (accepted_drag_drop && (flags & ImGuiColorEditFlags_InputHSV))
                ColorConvertRGBtoHSV(col[0], col[1], col[2], col[0], col[1], col[2]);
            EndDragDropTarget();
        }

        if (picker_active_window && g.ActiveId != 0 && g.ActiveIdWindow == picker_active_window)
            g.LastItemData.ID = g.ActiveId;

        if (value_changed && g.LastItemData.ID != 0)
            MarkItemEdited(g.LastItemData.ID);

        return value_changed;
    }

    static void RenderArrowsForVerticalBar(ImDrawList* draw_list, ImVec2 pos, ImVec2 half_sz, float bar_w, float alpha)
    {
        ImU32 alpha8 = IM_F32_TO_INT8_SAT(alpha);
        ImGui::RenderArrowPointingAt(draw_list, ImVec2(pos.x + half_sz.x + 1, pos.y), ImVec2(half_sz.x + 2, half_sz.y + 1), ImGuiDir_Right, IM_COL32(0, 0, 0, alpha8));
        ImGui::RenderArrowPointingAt(draw_list, ImVec2(pos.x + half_sz.x, pos.y), half_sz, ImGuiDir_Right, IM_COL32(255, 255, 255, alpha8));
        ImGui::RenderArrowPointingAt(draw_list, ImVec2(pos.x + bar_w - half_sz.x - 1, pos.y), ImVec2(half_sz.x + 2, half_sz.y + 1), ImGuiDir_Left, IM_COL32(0, 0, 0, alpha8));
        ImGui::RenderArrowPointingAt(draw_list, ImVec2(pos.x + bar_w - half_sz.x, pos.y), half_sz, ImGuiDir_Left, IM_COL32(255, 255, 255, alpha8));
    }

    struct picker_state
    {
        float hue_bar;
        float alpha_bar;
        float circle;
        ImVec2 circle_move;
    };

    static void DrawPickerSwatch(ImDrawList* draw_list, const ImRect& bb, const ImVec4& color, float rounding, float outline_alpha = 0.55f)
    {
        const float inset = 1.0f * c::scale;
        draw_list->AddRectFilled(bb.Min, bb.Max, IM_COL32(0, 0, 0, 185), rounding);
        RenderColorRectWithAlphaCheckerboard(
            draw_list,
            bb.Min + ImVec2(inset, inset),
            bb.Max - ImVec2(inset, inset),
            GetColorU32(color),
            ImMin(20.0f * c::scale, bb.GetHeight()) / 2.35f,
            ImVec2(0.0f, 0.0f),
            ImMax(0.0f, rounding - inset)
        );
        draw_list->AddRect(bb.Min, bb.Max, GetColorU32(WithAlpha(c::widget::outlinecolor, outline_alpha)), rounding, 0, 1.0f * c::scale);
    }

    static void DrawPickerPillHandle(ImDrawList* draw_list, const ImVec2& center, const ImVec2& half_size, const ImVec4& fill)
    {
        const float rounding = half_size.x + 1.0f * c::scale;
        draw_list->AddRectFilled(
            center - half_size - ImVec2(1.0f * c::scale, 1.0f * c::scale),
            center + half_size + ImVec2(1.0f * c::scale, 1.0f * c::scale),
            IM_COL32(0, 0, 0, 165),
            rounding
        );
        draw_list->AddRectFilled(center - half_size, center + half_size, GetColorU32(fill), rounding);
    }

    bool ColorPicker4(const char* label, float col[4], ImGuiColorEditFlags flags, const float* ref_col)
    {
        ImGuiContext& g = *GImGui;
        ImGuiWindow* window = GetCurrentWindow();
        if (window->SkipItems)
            return false;

        ImDrawList* draw_list = window->DrawList;
        ImGuiStyle& style = g.Style;
        ImGuiIO& io = g.IO;

        const float width = CalcItemWidth();
        g.NextItemData.ClearFlags();

        PushID(label);
        BeginGroup();

        if (!(flags & ImGuiColorEditFlags_NoSidePreview))
            flags |= ImGuiColorEditFlags_NoSmallPreview;

        if (!(flags & ImGuiColorEditFlags_NoOptions))
            ColorPickerOptionsPopup(col, flags);

        if (!(flags & ImGuiColorEditFlags_PickerMask_))
            flags |= ((g.ColorEditOptions & ImGuiColorEditFlags_PickerMask_) ? g.ColorEditOptions : ImGuiColorEditFlags_DefaultOptions_) & ImGuiColorEditFlags_PickerMask_;
        if (!(flags & ImGuiColorEditFlags_InputMask_))
            flags |= ((g.ColorEditOptions & ImGuiColorEditFlags_InputMask_) ? g.ColorEditOptions : ImGuiColorEditFlags_DefaultOptions_) & ImGuiColorEditFlags_InputMask_;
        IM_ASSERT(ImIsPowerOfTwo(flags & ImGuiColorEditFlags_PickerMask_));
        IM_ASSERT(ImIsPowerOfTwo(flags & ImGuiColorEditFlags_InputMask_));
        if (!(flags & ImGuiColorEditFlags_NoOptions))
            flags |= (g.ColorEditOptions & ImGuiColorEditFlags_AlphaBar);

        int components = (flags & ImGuiColorEditFlags_NoAlpha) ? 3 : 4;
        bool alpha_bar = (flags & ImGuiColorEditFlags_AlphaBar) && !(flags & ImGuiColorEditFlags_NoAlpha);
        const float shell_padding = 10.0f * c::scale;
        const float panel_rounding = 8.0f * c::scale;
        const float preview_height = 30.0f * c::scale;
        const float row_gap = 8.0f * c::scale;
        const float bar_height = 14.0f * c::scale;
        const float picker_width = ImClamp(width, 150.0f * c::scale, 210.0f * c::scale);
        const float sv_width = picker_width;
        const float sv_height = ImMax(104.0f * c::scale, picker_width * 0.66f);
        const float alpha_row_height = alpha_bar ? (row_gap + bar_height) : 0.0f;
        const ImVec2 panel_pos = window->DC.CursorPos;
        const ImVec2 panel_size(picker_width + shell_padding * 2.0f, preview_height + row_gap + sv_height + row_gap + bar_height + alpha_row_height + shell_padding * 2.0f);
        const ImRect panel_bb(panel_pos, panel_pos + panel_size);
        const ImVec2 info_pos = panel_bb.Min + ImVec2(shell_padding, shell_padding);
        const ImVec2 picker_pos = info_pos + ImVec2(0.0f, preview_height + row_gap);
        const ImVec2 hue_bar_pos = picker_pos + ImVec2(0.0f, sv_height + row_gap);
        const ImVec2 alpha_bar_pos = hue_bar_pos + ImVec2(0.0f, bar_height + row_gap);
        const float bars_width = picker_width;
        Dummy(panel_size);

        float backup_initial_col[4];
        memcpy(backup_initial_col, col, components * sizeof(float));

        float H = col[0], S = col[1], V = col[2];
        float R = col[0], G = col[1], B = col[2];
        if (flags & ImGuiColorEditFlags_InputRGB)
        {
            ColorConvertRGBtoHSV(R, G, B, H, S, V);
            ColorEditRestoreHS(col, &H, &S, &V);
        }
        else if (flags & ImGuiColorEditFlags_InputHSV)
        {
            ColorConvertHSVtoRGB(H, S, V, R, G, B);
        }

        bool value_changed = false, value_changed_h = false, value_changed_sv = false;

        PushItemFlag(ImGuiItemFlags_NoNav, true);

        SetCursorScreenPos(picker_pos);
        InvisibleButton("sv", ImVec2(sv_width, sv_height));
        if (IsItemActive())
        {
            S = ImSaturate((io.MousePos.x - picker_pos.x) / ImMax(1.0f, sv_width - 1.0f));
            V = 1.0f - ImSaturate((io.MousePos.y - picker_pos.y) / ImMax(1.0f, sv_height - 1.0f));

            if (g.ColorEditSavedColor == ColorConvertFloat4ToU32(ImVec4(col[0], col[1], col[2], 0)))
                H = g.ColorEditSavedHue;
            value_changed = value_changed_sv = true;
        }

        SetCursorScreenPos(hue_bar_pos);
        InvisibleButton("hue", ImVec2(bars_width, bar_height));
        if (IsItemActive())
        {
            H = 1.f - ImSaturate((io.MousePos.x - hue_bar_pos.x) / ImMax(1.0f, bars_width - 1.0f));
            value_changed = value_changed_h = true;
        }

        if (alpha_bar)
        {
            SetCursorScreenPos(alpha_bar_pos);
            InvisibleButton("alpha", ImVec2(bars_width, bar_height));
            if (IsItemActive())
            {
                col[3] = ImSaturate((io.MousePos.x - alpha_bar_pos.x) / ImMax(1.0f, bars_width - 1.0f));
                value_changed = true;
            }
        }
        PopItemFlag();

        if (value_changed_h || value_changed_sv)
        {
            if (flags & ImGuiColorEditFlags_InputRGB)
            {
                ColorConvertHSVtoRGB(H, S, V, col[0], col[1], col[2]);
                g.ColorEditSavedHue = H;
                g.ColorEditSavedSat = S;
                g.ColorEditSavedColor = ColorConvertFloat4ToU32(ImVec4(col[0], col[1], col[2], 0));
            }
            else if (flags & ImGuiColorEditFlags_InputHSV)
            {
                col[0] = H;
                col[1] = S;
                col[2] = V;
            }
        }

        bool value_changed_fix_hue_wrap = false;

        if (value_changed_fix_hue_wrap && (flags & ImGuiColorEditFlags_InputRGB))
        {
            float new_H, new_S, new_V;
            ColorConvertRGBtoHSV(col[0], col[1], col[2], new_H, new_S, new_V);
            if (new_H <= 0 && H > 0)
            {
                if (new_V <= 0 && V != new_V)
                    ColorConvertHSVtoRGB(H, S, new_V <= 0 ? V * 0.5f : new_V, col[0], col[1], col[2]);
                else if (new_S <= 0)
                    ColorConvertHSVtoRGB(H, new_S <= 0 ? S * 0.5f : new_S, new_V, col[0], col[1], col[2]);
            }
        }

        if (value_changed)
        {
            if (flags & ImGuiColorEditFlags_InputRGB)
            {
                R = col[0];
                G = col[1];
                B = col[2];
                ColorConvertRGBtoHSV(R, G, B, H, S, V);
                ColorEditRestoreHS(col, &H, &S, &V);
            }
            else if (flags & ImGuiColorEditFlags_InputHSV)
            {
                H = col[0];
                S = col[1];
                V = col[2];
                ColorConvertHSVtoRGB(H, S, V, R, G, B);
            }
        }
        ImU32 user_col32_striped_of_alpha = ColorConvertFloat4ToU32(ImVec4(R, G, B, style.Alpha));

        const int style_alpha8 = IM_F32_TO_INT8_SAT(style.Alpha);
        const ImU32 col_black = IM_COL32(0, 0, 0, style_alpha8);
        const ImU32 col_white = IM_COL32(255, 255, 255, style_alpha8);
        const ImU32 col_midgrey = IM_COL32(128, 128, 128, style_alpha8);
        const ImU32 col_hues[7] = { IM_COL32(255,0,0,style_alpha8), IM_COL32(255,0,255,style_alpha8), IM_COL32(0,0,255,style_alpha8),IM_COL32(0,255,255,style_alpha8), IM_COL32(0,255,0,style_alpha8), IM_COL32(255,255,0,style_alpha8), IM_COL32(255,0,0,style_alpha8) };

        ImVec4 hue_color_f(1, 1, 1, style.Alpha); ColorConvertHSVtoRGB(H, 1, 1, hue_color_f.x, hue_color_f.y, hue_color_f.z);
        ImU32 hue_color32 = ColorConvertFloat4ToU32(hue_color_f);

        ImVec2 sv_cursor_pos;
        draw_list->AddRectFilled(panel_bb.Min, panel_bb.Max, IM_COL32(0, 0, 0, 214), panel_rounding);
        draw_list->AddRectFilledMultiColor(
            panel_bb.Min + ImVec2(1.0f * c::scale, 1.0f * c::scale),
            ImVec2(panel_bb.Max.x - 1.0f * c::scale, panel_bb.Min.y + preview_height + shell_padding + row_gap),
            GetColorU32(WithAlpha(c::accent, 0.08f)),
            GetColorU32(WithAlpha(c::accent, 0.03f)),
            GetColorU32(ImVec4(0.0f, 0.0f, 0.0f, 0.0f)),
            GetColorU32(ImVec4(0.0f, 0.0f, 0.0f, 0.0f))
        );

        const ImVec4 current_color(R, G, B, components == 4 ? col[3] : 1.0f);
        char hex_buf[16];
        ImFormatString(hex_buf, IM_ARRAYSIZE(hex_buf), "#%02X%02X%02X", ImClamp(IM_F32_TO_INT8_UNBOUND(R), 0, 255), ImClamp(IM_F32_TO_INT8_UNBOUND(G), 0, 255), ImClamp(IM_F32_TO_INT8_UNBOUND(B), 0, 255));
        char alpha_buf[16];
        ImFormatString(alpha_buf, IM_ARRAYSIZE(alpha_buf), "%d%%", ImClamp((int)IM_ROUND((components == 4 ? col[3] : 1.0f) * 100.0f), 0, 100));
        const float current_swatch_width = 42.0f * c::scale;
        const float ref_swatch_width = ref_col ? 28.0f * c::scale : 0.0f;
        const float swatch_gap = ref_col ? 6.0f * c::scale : 0.0f;
        const ImRect current_swatch_bb(
            ImVec2(panel_bb.Max.x - shell_padding - current_swatch_width, info_pos.y),
            ImVec2(panel_bb.Max.x - shell_padding, info_pos.y + preview_height)
        );
        if (ref_col != NULL)
        {
            const ImRect ref_swatch_bb(
                ImVec2(current_swatch_bb.Min.x - swatch_gap - ref_swatch_width, info_pos.y),
                ImVec2(current_swatch_bb.Min.x - swatch_gap, info_pos.y + preview_height)
            );
            DrawPickerSwatch(draw_list, ref_swatch_bb, ImVec4(ref_col[0], ref_col[1], ref_col[2], components == 4 ? ref_col[3] : 1.0f), 6.0f * c::scale, 0.38f);
            draw_list->AddText(ref_swatch_bb.Min + ImVec2(8.0f * c::scale, (preview_height - CalcTextSize("R").y) * 0.5f - 1.0f * c::scale), GetColorU32(WithAlpha(c::text::text, 0.58f)), "R");
        }
        DrawPickerSwatch(draw_list, current_swatch_bb, current_color, 6.0f * c::scale);
        draw_list->AddText(info_pos, GetColorU32(WithAlpha(c::text::text, 0.5f)), "COLOR");
        draw_list->AddText(info_pos + ImVec2(0.0f, 15.0f * c::scale), GetColorU32(c::text::text_active), hex_buf);
        draw_list->AddText(info_pos + ImVec2(picker_width - current_swatch_width - ref_swatch_width - swatch_gap - 34.0f * c::scale, 15.0f * c::scale), GetColorU32(WithAlpha(c::text::text, 0.78f)), alpha_buf);

        const int vtx_idx_0 = draw_list->VtxBuffer.Size;
        draw_list->AddRectFilled(picker_pos, picker_pos + ImVec2(sv_width, sv_height), col_white, 6.0f * c::scale);
        const int vtx_idx_1 = draw_list->VtxBuffer.Size;
        ShadeVertsLinearColorGradientKeepAlpha(draw_list, vtx_idx_0, vtx_idx_1, picker_pos, picker_pos + ImVec2(sv_width, 0.0f), col_white, hue_color32);

        draw_list->AddRectFilledMultiColor(picker_pos, picker_pos + ImVec2(sv_width, sv_height), 0, 0, col_black, col_black, 6.0f * c::scale);
        draw_list->AddRect(picker_pos, picker_pos + ImVec2(sv_width, sv_height), GetColorU32(WithAlpha(c::widget::outlinecolor, 0.55f)), 6.0f * c::scale, 0, 1.0f * c::scale);

        sv_cursor_pos.x = ImClamp(IM_ROUND(picker_pos.x + ImSaturate(S) * sv_width), picker_pos.x + 2.0f * c::scale, picker_pos.x + sv_width - 2.0f * c::scale);
        sv_cursor_pos.y = ImClamp(IM_ROUND(picker_pos.y + ImSaturate(1 - V) * sv_height), picker_pos.y + 2.0f * c::scale, picker_pos.y + sv_height - 2.0f * c::scale);

        static std::map<ImGuiID, picker_state> anim;
        auto it_anim = anim.find(ImGui::GetID(label));

        if (it_anim == anim.end())
        {
            anim.insert({ ImGui::GetID(label), picker_state() });
            it_anim = anim.find(ImGui::GetID(label));
        }

        const ImRect hue_outer(hue_bar_pos, hue_bar_pos + ImVec2(bars_width, bar_height));
        draw_list->AddRectFilled(hue_outer.Min, hue_outer.Max, IM_COL32(0, 0, 0, 168), bar_height * 0.5f);
        const ImRect hue_inner(hue_outer.Min + ImVec2(1.0f * c::scale, 1.0f * c::scale), hue_outer.Max - ImVec2(1.0f * c::scale, 1.0f * c::scale));
        for (int i = 0; i < 6; ++i)
        {
            const float segment_min_x = hue_inner.Min.x + (hue_inner.GetWidth() / 6.0f) * i;
            const float segment_max_x = hue_inner.Min.x + (hue_inner.GetWidth() / 6.0f) * (i + 1);
            draw_list->AddRectFilledMultiColor(
                ImVec2(segment_min_x, hue_inner.Min.y),
                ImVec2(segment_max_x, hue_inner.Max.y),
                col_hues[i],
                col_hues[i + 1],
                col_hues[i + 1],
                col_hues[i]
            );
        }
        draw_list->AddRect(hue_outer.Min, hue_outer.Max, GetColorU32(WithAlpha(c::widget::outlinecolor, 0.55f)), bar_height * 0.5f, 0, 1.0f * c::scale);

        float bar0_line_x = IM_ROUND(hue_bar_pos.x + (1.0f - H) * bars_width);
        bar0_line_x = ImClamp(bar0_line_x, hue_outer.Min.x + 3.0f * c::scale, hue_outer.Max.x - 3.0f * c::scale);

        it_anim->second.hue_bar = ImLerp(it_anim->second.hue_bar, bar0_line_x - hue_bar_pos.x, g.IO.DeltaTime * 24.f);
        DrawPickerPillHandle(GetForegroundDrawList(), ImVec2(it_anim->second.hue_bar + hue_bar_pos.x, hue_outer.GetCenter().y), ImVec2(3.0f * c::scale, 8.0f * c::scale), AccentLift(0.18f));

        it_anim->second.circle_move = ImLerp(it_anim->second.circle_move, sv_cursor_pos - picker_pos, g.IO.DeltaTime * 24.f);
        it_anim->second.circle = ImLerp(it_anim->second.circle, value_changed_sv ? 5.0f * c::scale : 6.5f * c::scale, g.IO.DeltaTime * 24.f);

        const ImVec2 animated_cursor = picker_pos + it_anim->second.circle_move;
        GetForegroundDrawList()->AddCircleFilled(animated_cursor, it_anim->second.circle + 1.3f * c::scale, IM_COL32(0, 0, 0, 150), 30);
        GetForegroundDrawList()->AddCircleFilled(animated_cursor, it_anim->second.circle - 1.2f * c::scale, GetColorU32(current_color), 30);
        GetForegroundDrawList()->AddCircle(animated_cursor, it_anim->second.circle, IM_COL32(255, 255, 255, 230), 30, 1.4f * c::scale);

        if (alpha_bar)
        {
            const float alpha = ImSaturate(col[3]);
            const ImRect alpha_outer(alpha_bar_pos, alpha_bar_pos + ImVec2(bars_width, bar_height));
            const ImRect alpha_inner(alpha_outer.Min + ImVec2(1.0f * c::scale, 1.0f * c::scale), alpha_outer.Max - ImVec2(1.0f * c::scale, 1.0f * c::scale));
            draw_list->AddRectFilled(alpha_outer.Min, alpha_outer.Max, IM_COL32(0, 0, 0, 168), bar_height * 0.5f);
            RenderColorRectWithAlphaCheckerboard(draw_list, alpha_inner.Min, alpha_inner.Max, IM_COL32(255, 255, 255, 0), ImMin(18.0f * c::scale, alpha_inner.GetHeight()) / 2.25f, ImVec2(0.0f, 0.0f), ImMax(0.0f, bar_height * 0.5f - 1.0f * c::scale));
            draw_list->AddRectFilledMultiColor(
                alpha_inner.Min,
                alpha_inner.Max,
                GetColorU32(ImVec4(R, G, B, 0.0f)),
                GetColorU32(ImVec4(R, G, B, 1.0f)),
                GetColorU32(ImVec4(R, G, B, 1.0f)),
                GetColorU32(ImVec4(R, G, B, 0.0f))
            );
            draw_list->AddRect(alpha_outer.Min, alpha_outer.Max, GetColorU32(WithAlpha(c::widget::outlinecolor, 0.55f)), bar_height * 0.5f, 0, 1.0f * c::scale);

            float bar1_line_x = IM_ROUND(alpha_bar_pos.x + alpha * bars_width);
            bar1_line_x = ImClamp(bar1_line_x, alpha_outer.Min.x + 3.0f * c::scale, alpha_outer.Max.x - 3.0f * c::scale);
            it_anim->second.alpha_bar = ImLerp(it_anim->second.alpha_bar, bar1_line_x - alpha_bar_pos.x, g.IO.DeltaTime * 24.f);
            DrawPickerPillHandle(GetForegroundDrawList(), ImVec2(it_anim->second.alpha_bar + alpha_bar_pos.x, alpha_outer.GetCenter().y), ImVec2(3.0f * c::scale, 8.0f * c::scale), AccentLift(0.18f));
        }

        EndGroup();

        if (value_changed && memcmp(backup_initial_col, col, components * sizeof(float)) == 0) value_changed = false;
        if (value_changed) MarkItemEdited(g.LastItemData.ID);

        PopID();
        return value_changed;
    }

    bool ColorButton(const char* desc_id, const ImVec4& col, ImGuiColorEditFlags flags, const ImVec2& size_arg)
    {
        ImGuiWindow* window = GetCurrentWindow();
        if (window->SkipItems)
            return false;

        ImGuiContext& g = *GImGui;
        const ImGuiID id = window->GetID(desc_id);
        const float default_size = GetFrameHeight() * c::scale;
        const ImVec2 pos = window->DC.CursorPos;
        const float width = GetContentRegionMax().x - ImGui::GetStyle().WindowPadding.x;
        const ImRect rect(pos, pos + ImVec2(width, 32 * c::scale));

        const ImRect clickable(rect.Min + ImVec2(width - 47 * c::scale, 7 * c::scale), rect.Max - ImVec2(7 * c::scale, 7 * c::scale));

        ItemSize(ImRect(rect.Min, rect.Max - ImVec2(0, 0)));
        if (!ItemAdd(rect, id)) return false;

        bool hovered, held, pressed = ButtonBehavior(rect, id, &hovered, &held);

        if (flags & ImGuiColorEditFlags_NoAlpha) flags &= ~(ImGuiColorEditFlags_AlphaPreview | ImGuiColorEditFlags_AlphaPreviewHalf);

        ImVec4 col_rgb = col;
        if (flags & ImGuiColorEditFlags_InputHSV) ColorConvertHSVtoRGB(col_rgb.x, col_rgb.y, col_rgb.z, col_rgb.x, col_rgb.y, col_rgb.z);

        const float swatch_rounding = 6.0f * c::scale;
        window->DrawList->AddRectFilled(clickable.Min, clickable.Max, IM_COL32(0, 0, 0, 185), swatch_rounding);
        RenderColorRectWithAlphaCheckerboard(
            window->DrawList,
            clickable.Min + ImVec2(1.0f * c::scale, 1.0f * c::scale),
            clickable.Max - ImVec2(1.0f * c::scale, 1.0f * c::scale),
            GetColorU32(col_rgb),
            ImMin(20.0f * c::scale, clickable.GetHeight()) / 2.35f,
            ImVec2(0.0f, 0.0f),
            ImMax(0.0f, swatch_rounding - 1.0f * c::scale)
        );
        window->DrawList->AddRect(clickable.Min, clickable.Max, GetColorU32(hovered ? AccentLift(0.08f, 0.58f) : WithAlpha(c::widget::outlinecolor, 0.55f)), swatch_rounding, 0, 1.0f * c::scale);

        return pressed;
    }

    struct knob_state {
        float plus_float;
        int plus_int;
        ImVec4 background, circle, text;
        float slow_anim, circle_anim;
        float position;
    };

    bool KnobScalar(const char* label, ImGuiDataType data_type, void* p_data, const void* p_min, const void* p_max, const char* format, ImGuiSliderFlags flags)
    {
        ImGuiWindow* window = GetCurrentWindow();
        if (window->SkipItems) return false;

        ImGuiContext& g = *GImGui;
        const ImGuiStyle& style = g.Style;
        const ImGuiID id = window->GetID(label);
        const float w = GetContentRegionMax().x - style.WindowPadding.x;
        const float height = 32 * c::scale;

        const ImVec2 label_size = CalcTextSize(label, NULL, true);

        const ImRect frame_bb(window->DC.CursorPos + ImVec2(0, 0), window->DC.CursorPos + ImVec2(w, height));

        const ImRect slider_bb(window->DC.CursorPos + ImVec2(w - 30 * c::scale, 0), window->DC.CursorPos + ImVec2(w, 100 * c::scale));

        const ImRect total_bb(frame_bb.Min, frame_bb.Max + ImVec2(label_size.x > 0.0f ? label_size.x : 0.0f, 0.0f));

        const bool temp_input_allowed = (flags & ImGuiSliderFlags_NoInput) == 0;
        ItemSize(ImRect(total_bb.Min, total_bb.Max - ImVec2(0, 0)));

        if (!ItemAdd(total_bb, id, &frame_bb, temp_input_allowed ? ImGuiItemFlags_Inputable : 0)) return false;

        if (format == NULL) format = DataTypeGetInfo(data_type)->PrintFmt;

        bool hovered = ItemHoverable(frame_bb, id, g.LastItemData.InFlags), held, pressed = ButtonBehavior(frame_bb, id, &hovered, &held, NULL);

        ImRect grab_bb;

        static std::map<ImGuiID, knob_state> anim;
        auto it_anim = anim.find(id);

        if (it_anim == anim.end())
        {
            anim.insert({ id, knob_state() });
            it_anim = anim.find(id);
        }

        it_anim->second.circle_anim = ImLerp(it_anim->second.circle_anim, IsItemActive() ? 11.f * c::scale : 10.f * c::scale, g.IO.DeltaTime * 6.f);

        if ((flags & ImGuiSliderFlags_Integer) == 0) {
            if (*(float*)p_data > *(float*)p_max) *(float*)p_data = *(float*)p_max;
            if (*(float*)p_data < *(float*)p_min) *(float*)p_data = *(float*)p_min;
        }
        else
        {
            if (*(int*)p_data > *(int*)p_max) *(int*)p_data = *(int*)p_max;
            if (*(int*)p_data < *(int*)p_min) *(int*)p_data = *(int*)p_min;
        }

        it_anim->second.text = ImLerp(it_anim->second.text, g.ActiveId == id ? c::text::text_active : hovered ? c::text::text_hov : c::text::text, g.IO.DeltaTime * 6.f);

        const bool value_changed = DragBehavior(id, data_type, p_data, 0.f, p_min, p_max, format, NULL);

        if (value_changed) MarkItemEdited(id);

        char value_buf[64];
        const char* value_buf_end = value_buf + DataTypeFormatString(value_buf, IM_ARRAYSIZE(value_buf), data_type, p_data, format);

        float radius = 10.f * c::scale;
        float thickness = 3.f * c::scale;

        it_anim->second.position = ImLerp(it_anim->second.position, *static_cast<float*>(p_data) / *reinterpret_cast<const float*>(p_max) * 6.25f, ImGui::GetIO().DeltaTime * 18.f);

        GetWindowDrawList()->PathClear();
        GetWindowDrawList()->PathArcTo(ImVec2(frame_bb.Max.x + radius - 22.f * c::scale, frame_bb.Min.y + (height / 2)), radius, 0.f, 2.f * IM_PI, 40.f);
        GetWindowDrawList()->PathStroke(GetColorU32(c::elements::background), 0, thickness);

        GetWindowDrawList()->PathClear();
        GetWindowDrawList()->PathArcTo(ImVec2(frame_bb.Max.x + radius - 22.f * c::scale, frame_bb.Min.y + (height / 2)), radius, IM_PI * 1.5f, IM_PI * 1.5f + it_anim->second.position, 40.f);
        GetWindowDrawList()->PathStroke(GetColorU32(c::accent), 0, thickness);

        GetWindowDrawList()->AddCircleFilled(ImVec2(frame_bb.Max.x + radius - 22.f * c::scale + ImCos(IM_PI * 1.5f + it_anim->second.position) * radius, frame_bb.Min.y + (height / 2) + ImSin(IM_PI * 1.5f + it_anim->second.position) * radius), 2.f * c::scale, GetColorU32(c::text::text_active));

        GetWindowDrawList()->AddText(ImVec2(frame_bb.Max.x - (40 * c::scale + CalcTextSize(value_buf).x), frame_bb.Min.y + (height - CalcTextSize(value_buf).y) / 2), GetColorU32(c::text::text), value_buf);

        GetWindowDrawList()->AddText(ImVec2(frame_bb.Max.x - w, frame_bb.Min.y + (height - CalcTextSize(value_buf).y) / 2), GetColorU32(it_anim->second.text), label);

        return value_changed;
    }

    bool KnobFloat(const char* label, float* v, float v_min, float v_max, const char* format, ImGuiSliderFlags flags)
    {
        return KnobScalar(label, ImGuiDataType_Float, v, &v_min, &v_max, format, flags);
    }

    bool KnobInt(const char* label, int* v, int v_min, int v_max, const char* format, ImGuiSliderFlags flags)
    {
        return KnobScalar(label, ImGuiDataType_S32, v, &v_min, &v_max, format, flags | ImGuiSliderFlags_Integer);
    }

    struct slider_state {
        ImVec4 background, circle, text;
        float position, slow;
        float hover_t;
        float active_t;
        float value_alpha;
    };

    bool SliderScalar(const char* label, ImGuiDataType data_type, void* p_data, const void* p_min, const void* p_max, const char* format, ImGuiSliderFlags flags)
    {
        // Compact reference-style row (imgui_edited.cpp SliderScalar): label on
        // the left, value right-aligned, and a short accent track on the right
        // with a bright grab dot instead of a full-width bar.
        ImGuiWindow* window = GetCurrentWindow();
        if (window->SkipItems) return false;

        ImGuiContext& g = *GImGui;
        const ImGuiID id = window->GetID(label);
        const ImGuiStyle& style = g.Style;
        const char* label_end = FindRenderedTextEnd(label);
        const float w = GetContentRegionMax().x - style.WindowPadding.x;
        const ImVec2 label_size = CalcTextSize(label, label_end, true);
        if (format == NULL) format = DataTypeGetInfo(data_type)->PrintFmt;

        const float row_h   = 22.0f;
        const float track_w = 120.0f;
        const float track_h = 7.0f;

        const ImVec2 pos = window->DC.CursorPos;
        const ImRect total_bb(pos, pos + ImVec2(w, row_h));
        const ImRect hit_bb(ImVec2(total_bb.Max.x - track_w, pos.y),
                            ImVec2(total_bb.Max.x, pos.y + row_h));
        const ImRect slider_bb(ImVec2(hit_bb.Min.x, pos.y + (row_h - track_h) * 0.5f),
                               ImVec2(hit_bb.Max.x, pos.y + (row_h + track_h) * 0.5f));

        const bool temp_input_allowed = (flags & ImGuiSliderFlags_NoInput) == 0;
        ItemSize(total_bb, 0.0f);
        if (!ItemAdd(total_bb, id, &hit_bb, temp_input_allowed ? ImGuiItemFlags_Inputable : 0)) return false;

        char value_buf[64];
        DataTypeFormatString(value_buf, IM_ARRAYSIZE(value_buf), data_type, p_data, format);
        const ImVec2 value_size = CalcTextSize(value_buf);

        bool hovered = ItemHoverable(hit_bb, id, g.LastItemData.InFlags), held, pressed = ButtonBehavior(hit_bb, id, &hovered, &held, NULL);

        bool temp_input_is_active = temp_input_allowed && TempInputIsActive(id);
        if (temp_input_is_active)
        {
            const bool is_clamp_input = (flags & ImGuiSliderFlags_AlwaysClamp) != 0;
            return TempInputScalar(total_bb, id, label, data_type, p_data, format,
                                   is_clamp_input ? p_min : NULL, is_clamp_input ? p_max : NULL);
        }

        static std::map<ImGuiID, slider_state> anim;
        auto it_anim = anim.find(id);
        if (it_anim == anim.end())
        {
            anim.insert({ id, slider_state() });
            it_anim = anim.find(id);
            it_anim->second.slow = slider_bb.Min.x;
        }

        const bool active_slider = IsItemActive();
        it_anim->second.text = ImLerp(it_anim->second.text,
            active_slider ? c::text::text_active : hovered ? c::text::text_hov : c::text::text,
            g.IO.DeltaTime * 8.f);
        it_anim->second.hover_t = ImLerp(it_anim->second.hover_t, (hovered || active_slider) ? 1.0f : 0.0f, g.IO.DeltaTime * 10.0f);
        if (hovered || active_slider) SetMouseCursor(ImGuiMouseCursor_Hand);

        ImRect grab_bb;
        const bool value_changed = SliderBehavior(slider_bb, id, data_type, p_data, p_min, p_max, format, flags, &grab_bb);
        if (value_changed) MarkItemEdited(id);
        DataTypeFormatString(value_buf, IM_ARRAYSIZE(value_buf), data_type, p_data, format);

        const float grab_x = ImClamp((float) grab_bb.Max.x, slider_bb.Min.x, slider_bb.Max.x);
        it_anim->second.slow = ImLerp(it_anim->second.slow, grab_x, g.IO.DeltaTime * 20.0f);

        ImDrawList* dl = GetWindowDrawList();
        dl->AddRectFilled(slider_bb.Min, slider_bb.Max, GetColorU32(c::elements::background), track_h * 0.5f);

        const float fill_x = ImClamp(it_anim->second.slow, slider_bb.Min.x, slider_bb.Max.x);
        if (fill_x > slider_bb.Min.x + 0.5f)
        {
            dl->AddRectFilledMultiColor(slider_bb.Min, ImVec2(fill_x, slider_bb.Max.y),
                                        GetColorU32(AccentShade(0.10f)), GetColorU32(AccentLift(0.02f)),
                                        GetColorU32(AccentLift(0.02f)), GetColorU32(AccentShade(0.10f)));
            dl->AddRectFilled(slider_bb.Min, ImVec2(slider_bb.Min.x + track_h, slider_bb.Max.y),
                              GetColorU32(AccentShade(0.10f)), track_h * 0.5f);
        }

        const ImVec2 knob_center(fill_x, (slider_bb.Min.y + slider_bb.Max.y) * 0.5f);
        if (it_anim->second.hover_t > 0.02f)
            dl->AddCircleFilled(knob_center, 9.0f,
                                GetColorU32(ImVec4(c::accent.x, c::accent.y, c::accent.z, 0.18f * it_anim->second.hover_t)), 26);
        dl->AddCircleFilled(knob_center, 6.0f, IM_COL32(238, 238, 242, 255), 26);

        dl->AddText(ImVec2(total_bb.Min.x, total_bb.Min.y + (row_h - label_size.y) * 0.5f),
                    GetColorU32(it_anim->second.text), label, label_end);
        dl->AddText(ImVec2(slider_bb.Min.x - 10.0f - value_size.x, total_bb.Min.y + (row_h - value_size.y) * 0.5f),
                    IM_COL32(148, 148, 156, 255), value_buf);

        return value_changed;
    }

    bool SliderFloat(const char* label, float* v, float v_min, float v_max, const char* format, ImGuiSliderFlags flags)
    {
        return SliderScalar(label, ImGuiDataType_Float, v, &v_min, &v_max, format, flags);
    }

    bool SliderInt(const char* label, int* v, int v_min, int v_max, const char* format, ImGuiSliderFlags flags)
    {
        return SliderScalar(label, ImGuiDataType_S32, v, &v_min, &v_max, format, flags);
    }

}
