#pragma once
#include "../ImGui/imgui.h"
#include "Keyboard.h"
#include <string>
#include <algorithm>
#include <map>
#include <vector>
#include <cmath>
#include <functional>
#include <array>
#include <cstdint>
#include <cstddef>
#include <cfloat>
#include <cstring>
#include "../ImGui/imgui_settings.h"
#include "../ImGui/custom_widgets.hpp"

#include "../Skin/Fields.h"
#include "../Skin/Thread.h"
#include "../Skin/Camo.h"

extern float menu[4];
extern char searchQuery[256];
extern bool showKeyboard;
extern ImFont* F50;
extern ImFont* JAAT;
namespace font {
    extern ImFont* inter_semibold;
}

const int MIN_SEARCH_LENGTH = 3;
static int skinSubTab = 0;
static float cursorBlinkTimer = 0.0f;
static bool cursorVisible = true;
static std::string activeInputID = "";

std::string ToLower(std::string str) {
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);
    return str;
}

struct checkbox_state
{
    ImVec4 background, circle, text, box;
    float slow_circle;
};

enum class SkinTier {
    None,
    Mythic,
    Legendary,
    Epic,
    Common
};

static SkinTier DetectSkinTier(const std::string& rawName) {
    if (rawName.find("[M]") != std::string::npos || rawName.find("[MYTHIC]") != std::string::npos) return SkinTier::Mythic;
    if (rawName.find("[L]") != std::string::npos || rawName.find("[LEGENDARY]") != std::string::npos) return SkinTier::Legendary;
    if (rawName.find("[E]") != std::string::npos || rawName.find("[EPIC]") != std::string::npos) return SkinTier::Epic;
    if (rawName.find("[C]") != std::string::npos || rawName.find("[COMMON]") != std::string::npos) return SkinTier::Common;
    return SkinTier::None;
}

static std::string StripTierTag(const std::string& rawName) {
    std::string out = rawName;
    const char* tags[] = {"[MYTHIC]", "[LEGENDARY]", "[EPIC]", "[COMMON]", "[M]", "[L]", "[E]", "[C]"};
    for (const char* tag : tags) {
        const std::string t(tag);
        size_t pos = out.find(t);
        if (pos != std::string::npos) {
            out.erase(pos, t.length());
            while (!out.empty() && out.front() == ' ')
                out.erase(out.begin());
            while (!out.empty() && out.back() == ' ')
                out.pop_back();
            break;
        }
    }
    return out;
}

std::string GetBaseWeaponName(const std::string& fullName) {
    size_t dashPos = fullName.find(" - ");
    if (dashPos == std::string::npos) return fullName;
    std::string base = fullName.substr(0, dashPos);
    size_t tagEnd = base.find("] ");
    if (tagEnd != std::string::npos && tagEnd + 2 < base.length())
        base = base.substr(tagEnd + 2);
    return base;
}

namespace ImGui {

// Optional color override for AstralInput (dmalogin login look). When nullptr,
// the legacy c:: palette is used so existing call sites are unaffected.
struct AstralInputStyle {
    ImVec4 bg;
    ImVec4 bgHovered;
    ImVec4 bgActive;
    ImVec4 border;
    ImVec4 borderActive;
    ImVec4 hint;
    ImVec4 text;
    ImVec4 accent;
    float  rounding;
};

bool AstralInput(const char* label, char* buf, size_t buf_size, const ImVec2& size = ImVec2(0, 0), const AstralInputStyle* sty = nullptr) {
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    ImGuiIO& io = g.IO;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);

    ImVec2 pos = window->DC.CursorPos;
    ImVec2 inputSize = size;
    if (inputSize.x <= 0.0f)
        inputSize.x = GetContentRegionAvail().x;
    if (inputSize.y <= 0.0f)
        inputSize.y = 50.0f;

    ImRect bb(pos, ImVec2(pos.x + inputSize.x, pos.y + inputSize.y));
    ItemSize(bb, style.FramePadding.y);
    if (!ItemAdd(bb, id))
        return false;

    bool hovered, held;
    ButtonBehavior(bb, id, &hovered, &held);
    bool clicked = hovered && IsMouseClicked(0);
    bool isActive = (g.ActiveId == id);

    if (clicked) {
        SetActiveID(id, window);
        SetFocusID(id, window);
        FocusWindow(window);
        activeInputID = label;
    }

    if (isActive) {
        cursorBlinkTimer += io.DeltaTime;
        if (cursorBlinkTimer >= 1.0f) {
            cursorBlinkTimer = 0.0f;
            cursorVisible = !cursorVisible;
        }
    } else {
        cursorBlinkTimer = 0.0f;
        cursorVisible = true;
    }

    const ImU32 bgColor = sty
        ? GetColorU32(isActive ? sty->bgActive : hovered ? sty->bgHovered : sty->bg)
        : GetColorU32(isActive ? c::elements::background_hovered : hovered ? c::button::background_hovered : c::widget::background);
    const ImU32 borderColor = sty
        ? GetColorU32(isActive ? sty->borderActive : sty->border)
        : GetColorU32(isActive ? c::accent : c::button::outline);
    const float inputRounding = sty ? sty->rounding : 4.0f;
    const ImVec4 textColor = sty ? sty->text : c::text::text_active;

    ImDrawList* draw_list = window->DrawList;

    draw_list->AddRectFilled(bb.Min, bb.Max, bgColor, inputRounding);
    draw_list->AddRect(bb.Min, bb.Max, borderColor, inputRounding, 0, 1.0f);

    std::string text = buf;
    ImVec2 textPos = ImVec2(bb.Min.x + 15.0f, bb.Min.y + (inputSize.y - GetFontSize()) * 0.5f);

    PushClipRect(ImVec2(bb.Min.x + 10.0f, bb.Min.y), ImVec2(bb.Max.x - 10.0f, bb.Max.y), true);

    if (text.empty() && !isActive) {
        ImVec4 hintColor = sty ? sty->hint : c::text::text_hov;
        if (!sty) hintColor.w *= 0.70f;
        draw_list->AddText(textPos, GetColorU32(hintColor), "Type here...");
    } else {
        draw_list->AddText(textPos, GetColorU32(textColor), text.c_str());

        if (isActive && cursorVisible) {
            ImVec2 textSize = CalcTextSize(text.c_str());
            float cursorX = textPos.x + textSize.x + 2.0f;
            float cursorY1 = textPos.y;
            float cursorY2 = textPos.y + GetFontSize();
            draw_list->AddLine(ImVec2(cursorX, cursorY1), ImVec2(cursorX, cursorY2),
                               GetColorU32(textColor), 2.0f);
        }
    }

    PopClipRect();

    if (isActive) {
        float animProgress = fmod(cursorBlinkTimer, 1.0f);
        float lineWidth = bb.GetWidth() * 0.3f;
        float lineStart = bb.Min.x + (bb.GetWidth() - lineWidth) * 0.5f;
        float lineEnd = lineStart + lineWidth;

        float alpha = 1.0f - animProgress;
        if (animProgress > 0.5f) {
            alpha = (1.0f - animProgress) * 2.0f;
        } else {
            alpha = animProgress * 2.0f;
        }

        ImVec4 lineColorF = sty ? sty->accent : c::accent;
        lineColorF.w = alpha;
        ImU32 lineColor = GetColorU32(lineColorF);
        draw_list->AddLine(ImVec2(lineStart, bb.Max.y - 2.0f),
                           ImVec2(lineEnd, bb.Max.y - 2.0f),
                           lineColor, 3.0f);
    }

    return false;
}

    bool CheckboxFullWidth(const char* label, bool* v)
    {
        return custom::Checkbox(label, v);
    }
}

static bool DrawTieredSkinCheckbox(const char* label, bool* value, std::function<void()> applySkin = nullptr) {
    if (!value) return false;

    std::string rawLabel = label;
    const SkinTier tier = DetectSkinTier(rawLabel);
    const std::string cleanLabel = StripTierTag(rawLabel);
    
    ImGui::PushID(label);
    
    const bool pressed = custom::Checkbox("##skin_checkbox", value);
    if (pressed && applySkin) {
        applySkin(); 
    }
    
    const bool hovered = ImGui::IsItemHovered();
    const ImVec2 rowMin = ImGui::GetItemRectMin();
    const ImVec2 rowMax = ImGui::GetItemRectMax();
    const float su = c::scale * c::widget_scale;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImFont* labelFont = font::inter_semibold ? font::inter_semibold : ImGui::GetFont();
    const float labelSize = labelFont->FontSize * 0.84f;

    float currentX = rowMin.x + 35.0f * su; 

    if (tier != SkinTier::None) {
        const char* badgeText = "";
        ImU32 badgeBg = IM_COL32(34, 46, 41, 170);
        ImU32 badgeFg = IM_COL32(210, 226, 218, 245);

        if (tier == SkinTier::Mythic) {
            badgeText = "MYTHIC";
            badgeBg = IM_COL32(145, 30, 34, 190);
            badgeFg = IM_COL32(255, 184, 184, 250);
        } else if (tier == SkinTier::Legendary) {
            badgeText = "LEGENDARY";
            badgeBg = IM_COL32(137, 79, 16, 190);
            badgeFg = IM_COL32(255, 217, 151, 250);
        } else if (tier == SkinTier::Epic) {
            badgeText = "EPIC";
            badgeBg = IM_COL32(90, 52, 150, 190);
            badgeFg = IM_COL32(227, 196, 255, 250);
        } else if (tier == SkinTier::Common) {
            badgeText = "COMMON";
            badgeBg = IM_COL32(23, 74, 40, 200);     
            badgeFg = IM_COL32(74, 222, 128, 255);    
        }
        
        const ImVec2 badgeTextSize = labelFont->CalcTextSizeA(labelSize, FLT_MAX, 0.0f, badgeText);
        const float badgePadX = 6.0f * su;
        const float badgePadY = 2.0f * su;
        const float badgeH = badgeTextSize.y + (badgePadY * 2.0f);
        const float badgeW = badgeTextSize.x + (badgePadX * 2.0f);

        const ImVec2 badgeMin(currentX, rowMin.y + (rowMax.y - rowMin.y - badgeH) * 0.5f);
        const ImVec2 badgeMax(badgeMin.x + badgeW, badgeMin.y + badgeH);

        dl->AddRectFilled(badgeMin, badgeMax, badgeBg, 4.0f * su);
        dl->AddText(labelFont, labelSize, ImVec2(badgeMin.x + badgePadX, badgeMin.y + badgePadY), badgeFg, badgeText);

        currentX += badgeW + (6.0f * su); 
    }
    
    const ImVec2 nameSize = labelFont->CalcTextSizeA(labelSize, FLT_MAX, 0.0f, cleanLabel.c_str());
    const float nameY = rowMin.y + (rowMax.y - rowMin.y - nameSize.y) * 0.5f;
    
    dl->AddText(
        labelFont,
        labelSize,
        ImVec2(currentX, nameY),
        hovered ? IM_COL32(240, 232, 248, 255) : IM_COL32(236, 240, 246, 255),
        cleanLabel.c_str()
    );
    
    ImGui::PopID();
    return pressed;
}

static bool SkinCheckbox(const char* label, bool* value, std::function<void()> applySkin = nullptr) {
    return DrawTieredSkinCheckbox(label, value, applySkin);
}

static bool WeaponSkinCheckbox(const char* label, bool* value, std::function<void()> applySkin = nullptr) {
    return DrawTieredSkinCheckbox(label, value, applySkin);
}

inline void SelectSnowboardSkin(int skinId)
{
    void* selectedSkin = FindVehicleSkinConfigById(skinId);
    if (!SkinPtr(selectedSkin))
        return;

    auto* fields = (VehicleSkinConfFields*)((uintptr_t)selectedSkin + 0x10);
    if (!SkinPtr(fields))
        return;

    activeVehicleSkins[31707110] = skinId;
    activeVehicleSkinsById[(int)fields->VehicleId] = skinId;
    activeVehicleSkinConfs[skinId] = selectedSkin;
}

inline void RenderSkinCategoryContent(int categoryIndex, bool drawSubTabs = false) {

    // --- Camo States ---
    static bool camoOff = true;
    static bool camoDiamond = false;
    static bool camoRedSprite = false; 
    static bool camoGold = false;
    static bool camoPlatinum = false;
    static bool camoDamascus = false;
    static bool camoAetherCrystal = false;
    static bool camoGlacialRipple = false;
    
    #define ID_DIAMOND 0x1D37F758 
    #define ID_RED_SPRITE 0x1D37F77E 
    #define ID_GOLD 0x1D37F755
    #define ID_PLATINUM 0x1D37F756
    #define ID_DAMASCUS 0x1D37F757
    #define ID_AETHER_CRYSTAL 0x1D37F77D
    #define ID_GLACIAL_RIPPLE 0x1D37F77F
    
    // The skin loader publishes into charData / itemData / watch / ... from its
    // own thread; hold the UI lock while we read them and while we apply a skin.
    std::lock_guard<std::mutex> skinDataLock(g_skinUiMutex);

    ImGui::BeginChild("SkinContent", ImVec2(0, 0));
    ImGui::Dummy(ImVec2(0, 8 * c::scale));
    
    if (drawSubTabs) {
        const float tabSpacing = 8.0f * c::scale;
        const float tabHeight = 34.0f * c::scale;
        // Pinalitan ang / 5.0f ng / 6.0f para magkasya ang 6 na sub-tabs sa isang linya
        const float tabWidth = ImMax(32.0f, (ImGui::GetContentRegionAvail().x - tabSpacing * 6.0f) / 7.0f);
        ImFont* subTabFont = font::inter_semibold ? font::inter_semibold : F50;
        
        if (subTabFont) ImGui::PushFont(subTabFont);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(tabSpacing, tabSpacing));
        
        if (custom::Page(skinSubTab == 0, "Character", ImVec2(tabWidth, tabHeight), false, false)) skinSubTab = 0;
        ImGui::SameLine();
        if (custom::Page(skinSubTab == 1, "Watch", ImVec2(tabWidth, tabHeight), false, false)) skinSubTab = 1;
        ImGui::SameLine();
        if (custom::Page(skinSubTab == 2, "Deadbox", ImVec2(tabWidth, tabHeight), false, false)) skinSubTab = 2;
        ImGui::SameLine();
        if (custom::Page(skinSubTab == 3, "Plane", ImVec2(tabWidth, tabHeight), false, false)) skinSubTab = 3;
        ImGui::SameLine();
        if (custom::Page(skinSubTab == 4, "Weapon", ImVec2(tabWidth, tabHeight), false, false)) skinSubTab = 4;
        ImGui::SameLine();
        if (custom::Page(skinSubTab == 5, "Camo", ImVec2(tabWidth, tabHeight), false, false)) skinSubTab = 5;
        ImGui::SameLine();
        if (custom::Page(skinSubTab == 6, "Snowboard", ImVec2(tabWidth, tabHeight), false, false)) skinSubTab = 6;
        if (subTabFont) ImGui::PopFont();
    }

    static char charSearchQuery[256] = "";
    static bool skinSearchWasActive = false;
    static bool charSearchWasActive = false;
    bool charActive = false;
    bool charHovered = false;
    
    ImGui::Indent(10.0f);
    float totalAvail = ImGui::GetContentRegionAvail().x - 20.0f;
    float halfWidth = (totalAvail - 10.0f) * 0.5f;
    float searchWidth = (skinSubTab == 0) ? halfWidth : totalAvail;
    
    ImGui::TextColored(c::text::text_active, "Search Skin:");
    if (skinSubTab == 0) {
        ImGui::SameLine(halfWidth + 20.0f);
        ImGui::TextColored(c::text::text_active, "Search Custom Char:");
    }
    
    ImGui::AstralInput("##SearchSkin", searchQuery, IM_ARRAYSIZE(searchQuery), ImVec2(searchWidth, 50));
    bool skinClicked = ImGui::IsItemClicked();
    bool skinActive  = ImGui::IsItemActive();
    bool skinHovered = ImGui::IsItemHovered();
    if (skinClicked || (!skinSearchWasActive && skinActive)) {
        activeInputID = "##SearchSkin";
        showKeyboard = true;
    }
    skinSearchWasActive = skinActive;
    
    if (skinSubTab == 0) {
        ImGui::SameLine(0.0f, 10.0f);
        ImGui::AstralInput("##SearchCustomChar", charSearchQuery, IM_ARRAYSIZE(charSearchQuery), ImVec2(halfWidth, 50));
        bool charClicked = ImGui::IsItemClicked();
        charActive = ImGui::IsItemActive();
        charHovered = ImGui::IsItemHovered();
        if (charClicked || (!charSearchWasActive && charActive)) {
            activeInputID = "##SearchCustomChar";
            showKeyboard = true;
        }
        charSearchWasActive = charActive;
    }
        
    if (showKeyboard && !skinActive && (skinSubTab != 0 || (!charSearchWasActive && !charActive && !charHovered)) && ImGui::IsMouseClicked(0)) {
        ImGuiIO& io = ImGui::GetIO();
        float screenHeight = io.DisplaySize.y;
        float keyboardHeight = screenHeight * 0.60f;
        if (ImGui::GetMousePos().y < screenHeight - keyboardHeight) {
            showKeyboard = false;
        }
    }

    custom::Separator_line();
    ImGui::Unindent(30.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 4));
    ImGui::Indent(20.0f);
    std::string searchLower = ToLower(searchQuery);
    int itemCount = 0;
    const char *itemType = "";
    switch (skinSubTab)
    {
    case 0:
        itemCount = charData.size();
        itemType = "Characters";
        break;
    case 1:
        itemCount = watch.size();
        itemType = "Watches";
        break;
    case 2:
        itemCount = deadboxF.size();
        itemType = "Deadboxes";
        break;
    case 3:
        itemCount = dropplane.size();
        itemType = "Planes";
        break;
    case 4:
        itemCount = itemData.size();
        itemType = "Weapons";
        break;
    case 5:
        itemCount = itemData.size();
        itemType = "Camo";
        break;
    case 6:
        itemCount = snowboardData.size();
        itemType = "Snowboards";
        break;
    }
    if (skinSubTab == 0 && !g_targetCharacters.empty()) {
        std::string charSearchLower = ToLower(charSearchQuery);
        std::vector<int> filteredIndices;
        std::vector<const char*> filteredNames;

        for (size_t i = 0; i < g_targetCharacters.size(); i++) {
            if (strlen(charSearchQuery) >= MIN_SEARCH_LENGTH) {
                if (ToLower(g_targetCharacters[i].name).find(charSearchLower) == std::string::npos) continue;
            }
            filteredIndices.push_back(i);
            filteredNames.push_back(g_targetCharacters[i].name.c_str());
        }

        if (!filteredNames.empty()) {
            int currentInFiltered = 0;
            for (size_t i = 0; i < filteredIndices.size(); i++) {
                if (filteredIndices[i] == g_selectedTargetCharIndex) { currentInFiltered = i; break; }
            }
            custom::BeginGroup();
            if (custom::Combo("Target Character:", &currentInFiltered, filteredNames.data(), filteredNames.size())) {
                g_selectedTargetCharIndex = filteredIndices[currentInFiltered];
            }
            custom::EndGroup();
        }
    }

    ImGui::BeginChild("##SkinList", ImVec2(0, ImGui::GetContentRegionAvail().y - 10), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);

    switch (skinSubTab) {
case 0:
    if (!charData.empty()) {

        for (const auto& getchar : charData) {

            if (strlen(searchQuery) >= MIN_SEARCH_LENGTH) {

                if (ToLower(getchar.charName).find(searchLower) == std::string::npos)
                    continue;
            }

            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 8));

            SkinCheckbox(getchar.charName.c_str(), &sBool[getchar.charName], [&]() {

                for (auto& pair : sBool) {

                    if (pair.first != getchar.charName)
                        pair.second = false;
                }

                if (!CharacterModelConfigInstance.empty() &&
                    !itemResourceConfigInstance.empty() &&
                    !RoleConfConfigInstance.empty() &&
                    !RolePackConfConfigInstance.empty()) {

                    if (g_targetCharacters.empty())
                        return;

                    if (g_selectedTargetCharIndex < 0 ||
                        (size_t)g_selectedTargetCharIndex >= g_targetCharacters.size())
                        g_selectedTargetCharIndex = 0;

                    auto& target = g_targetCharacters[g_selectedTargetCharIndex];

                    int selTraitor1P = (target.name == "Charly")
                                       ? 710001101
                                       : target.traitor1p;

                    int selTraitor3P = (target.name == "Charly")
                                       ? 710001102
                                       : target.traitor3p;

                    int selItemID = (target.name == "Charly")
                                    ? 100301208
                                    : target.itemID;

                    int selRoleID = (target.name == "Charly")
                                    ? 100301208
                                    : target.roleID;

                    int selPackID = target.rolepackID;

                    for (auto charModel : SkinSnapshot(CharacterModelConfigInstance)) {

                        if (!charModel)
                            continue;

                        auto* cf = (CharacterModelFields*)((uintptr_t)charModel + 0x18);

                        if (cf->Traitor1P == selTraitor1P ||
                            cf->Traitor3P == selTraitor3P ||
                            cf->Traitor1P == 710001101) {

                            cf->BRBagModel         = getchar.charModel[0];
                            cf->BRHeadModel        = getchar.charModel[1];
                            cf->BRLobby            = getchar.charModel[2];
                            cf->BRModel            = getchar.charModel[3];
                            cf->BindEffect1P       = getchar.charModel[4];
                            cf->ChangeClipEffect1P = getchar.charModel[5];
                            cf->DefaultModelID     = getchar.charModel[6];
                            cf->Guarder1P          = getchar.charModel[7];
                            cf->Guarder3P          = getchar.charModel[8];
                            cf->GuarderBagModel    = getchar.charModel[9];
                            cf->GuarderHeadModel   = getchar.charModel[10];
                            cf->GuarderLobby       = getchar.charModel[11];
                        }
                    }

                    // ITEM RESOURCE
                    for (auto itemRes : SkinSnapshot(itemResourceConfigInstance)) {

                        if (!itemRes)
                            continue;

                        auto* itf = (ItemResourceFields*)((uintptr_t)itemRes + 0x18);

                        if (itf->ID == selItemID ||
                            itf->ID == 100301208) {

                            itf->FxAssetID          = getchar.charRes[0];
                            itf->InventoryModelID   = getchar.charRes[1];
                            itf->ModelAssetIDRaw    = getchar.charRes[2];
                            // charRes is the only character-resource array
                            // available in the supplied character data (3 entries).
                            itf->UIMiniSpriteNameIndex  = getchar.charRes[0];
                            itf->UISmallSpriteNameIndex = getchar.charRes[1];
                            itf->UISpriteNameIndex      = getchar.charRes[2];

                            // The supplied character data has no 4th sprite-index
                            // value, so do not read charRes[3] or a nonexistent
                            // charRes2 member. Keep the square sprite unchanged.
                            // If your Fields.h defines a separate 4-element array,
                            // replace this line with that array's [3] value.

                        }
                    }

                    for (auto roles : SkinSnapshot(RoleConfConfigInstance)) {

                        if (!roles)
                            continue;

                        auto* rf = (RoleConfFields*)((uintptr_t)roles + 0x18);

                        if (rf->ID == selRoleID ||
                            rf->ID == 100301208) {

                            rf->roleLeftArmID       = getchar.charRole[0];
                            rf->roleFinalSuitID     = getchar.charRole[1];
                            rf->roleBasicHologramID = getchar.charRole[2];
                            rf->ColorID             = getchar.charRole[3];
                            rf->ColorSubID          = getchar.charRole[4];
                            rf->ShowRare            = getchar.charRole[5];
                            rf->RoleLvGroupID       = getchar.charRole[6];
                            rf->RolePackID          = getchar.charRole[7];
                            rf->LOCID_Name          = getchar.charRole2[0];
                        }
                    }

                    for (auto pack : SkinSnapshot(RolePackConfConfigInstance)) {

                        if (!pack)
                            continue;

                        auto* pf = (RolePackFields*)((uintptr_t)pack + 0x18);

                        if (pf->RolePackID == selPackID &&
                            pf->LobbySceneType == 0 &&
                            pf->LoadingFrame != 0) {

                            pf->EntryAnimID      = getchar.charPack[1];
                            pf->GestureId        = getchar.charPack[2];
                            pf->HandEffectUI     = getchar.charPack[3];
                            pf->LoadingFrame     = getchar.charPack[4];
                            pf->KillStreakSkinID = getchar.charPack[5];
                        }
                    }
                }
            });

            ImGui::PopStyleVar();
        }
    }
    else
    {
        ImGui::TextColored(ImVec4(1, 0.5f, 0.5f, 1), "No character skins loaded yet - bring the tab up once the game has loaded");
    }
    break;

case 1:
    for (const auto& w : watch) {

        if (strlen(searchQuery) >= MIN_SEARCH_LENGTH &&
            ToLower(w.watchname).find(searchLower) == std::string::npos)
            continue;

        if (w.watchname.find("Watch") == std::string::npos)
            continue;

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 8));

        SkinCheckbox(w.watchname.c_str(), &sBool[w.watchname], [&]() {

            if (g_targetCharacters.empty())
                return;

            const int targetIndex = (g_selectedTargetCharIndex >= 0 &&
                                     (size_t)g_selectedTargetCharIndex < g_targetCharacters.size())
                                    ? g_selectedTargetCharIndex
                                    : 0;

            int sT1P = (g_targetCharacters[targetIndex].name == "Charly")
                       ? 710001101
                       : g_targetCharacters[targetIndex].traitor1p;

            for (auto m : SkinSnapshot(CharacterModelConfigInstance)) {

                auto* cf = (CharacterModelFields*)((uintptr_t)m + 0x18);

                if (cf->Traitor1P == sT1P ||
                    cf->Traitor1P == 710001101) {

                    cf->BindEffect1P = w.watchvalue;
                }
            }
        });

        ImGui::PopStyleVar();
    }
    if (watch.empty())
        ImGui::TextColored(ImVec4(1, 0.5f, 0.5f, 1), "No watch skins loaded yet - bring the tab up once the game has loaded");
    break;

case 2:
    for (const auto& d : deadboxF) {

        if (strlen(searchQuery) >= MIN_SEARCH_LENGTH &&
            ToLower(d.deadname).find(searchLower) == std::string::npos)
            continue;

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 8));

        SkinCheckbox(d.deadname.c_str(), &sBool[d.deadname], [&]() {

            for (auto db : SkinSnapshot(BRDeadboxSkinConfigInstance)) {

                auto* df = (BRDeadboxSkinFields*)((uintptr_t)db + 0x10);

                if (df->ID == 180300004) {

                    df->ColorID            = d.dead[0];
                    df->DeadBoxEffectAsset = d.dead[1];
                    df->Flag               = d.dead[2];
                    df->FlagAsset          = d.dead[3];
                    df->ModelAsset3P       = d.dead[4];
                    df->ModelAssetUI       = d.dead[5];
                }
            }
        });

        ImGui::PopStyleVar();
    }
    if (deadboxF.empty())
        ImGui::TextColored(ImVec4(1, 0.5f, 0.5f, 1), "No deadbox skins loaded yet - bring the tab up once the game has loaded");
    break;

case 3:
    for (const auto& p : dropplane) {

        if (strlen(searchQuery) >= MIN_SEARCH_LENGTH &&
            ToLower(p.planename).find(searchLower) == std::string::npos)
            continue;

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 8));

        SkinCheckbox(p.planename.c_str(), &sBool[p.planename], [&]() {

            for (auto pl : SkinSnapshot(BRDropPlaneSkinConfigInstance)) {

                auto* dpf = (BRDropPlaneSkinFields*)((uintptr_t)pl + 0x10);

                if (dpf->ID == 0) {

                    dpf->ColorID          = p.plane[0];
                    dpf->ModelAsset1P     = p.plane[1];
                    dpf->ModelAsset3P     = p.plane[2];
                    dpf->ModelAssetCutScene = p.plane[3];
                    dpf->ModelAssetUI     = p.plane[4];
                    dpf->Priority         = p.plane[5];
                }
            }
        });

        ImGui::PopStyleVar();
    }
    if (dropplane.empty())
        ImGui::TextColored(ImVec4(1, 0.5f, 0.5f, 1), "No drop plane skins loaded yet - bring the tab up once the game has loaded");
    break;

    case 4:

        if (!itemData.empty())
        {
            auto applyWeaponSkin = [&](const auto& getitem) {
                int baseID = getitem.WeaponConf[0];
                int extraID = getitem.WeaponExtra[0];
                int skinID = getitem.WeaponConf[2];
                int itemID = getitem.Item2Inventory[0];
                int lootID = getitem.Item2Inventory[3];
                for (auto item : SkinSnapshot(itemInventoryInstance))
                {
                    if (!item)
                        continue;
                    item2Fields = (Item2InventoryFields *)((uintptr_t)item + 0x20);
                    if (item2Fields->ItemID == getitem.Item2Inventory[0] || item2Fields->ItemID == getitem.Item2Inventory[3])
                    {
                        item2Fields->WeaponAssetGroupID = getitem.Item2Inventory[1];
                        item2Fields->WeaponIconID = getitem.Item2Inventory[2];
                    }
                }
                for (auto conf : SkinSnapshot(weaponConfInstance))
                {
                    if (!conf)
                        continue;
                    weaponconfFields = (WeaponConfFields *)((uintptr_t)conf + 0x20);
                    if (baseID == weaponconfFields->ID ||
                            extraID == weaponconfFields->ID ||
                            skinID == weaponconfFields->ID ||
                            itemID == weaponconfFields->ID ||
                            lootID == weaponconfFields->ID)
                    {
                        weaponconfFields->DefWeaponSkinID = getitem.WeaponConf[2];
                        weaponconfFields->DefaultKillBrocast = getitem.WeaponConf[3];
                        weaponconfFields->ExternalUnVisible = true;

                        for (auto skinConf : SkinSnapshot(weaponConfInstance))
                        {
                            if (!skinConf)
                                continue;
                            WeaponConfFields *skinFields = (WeaponConfFields *)((uintptr_t)skinConf + 0x20);
                            if (skinFields->ID == getitem.WeaponConf[2])
                            {
                                weaponconfFields->LOCID_Name = skinFields->LOCID_Name;
                                break;
                            }
                        }
                        if (baseID == weaponconfFields->ID)
                        {
                            weaponconfFields->ColorID = getitem.WeaponConf[1];
                        }
                    }
                }
                if (getitem.WeaponExtra[4] > 0)
                {
                    activeKillEffects[baseID] = getitem.WeaponExtra[4];
                    activeKillEffects[extraID] = getitem.WeaponExtra[4];
                    activeKillEffects[skinID] = getitem.WeaponExtra[4];
                    activeKillEffects[itemID] = getitem.WeaponExtra[4];
                    activeKillEffects[lootID] = getitem.WeaponExtra[4];
                }
                for (auto extra : SkinSnapshot(weaponExtraInstance))
                {
                    if (!extra)
                        continue;
                    weaponextraFields = (WeaponConfExtraFields *)((uintptr_t)extra + 0x10);
                    if (weaponextraFields->ID == baseID ||
                            weaponextraFields->ID == extraID ||
                            weaponextraFields->ID == skinID ||
                            weaponextraFields->ID == itemID ||
                            weaponextraFields->ID == lootID)
                    {
                        if (getitem.itemName.find("[MYTHIC]") != std::string::npos)
                        {
                            weaponextraFields->DefaultMythicArmor = getitem.WeaponExtra[1];
                            if (getitem.itemName.find("[MYTHIC] AK117 - Memento Mori") != std::string::npos)
                            {
                                weaponextraFields->DefaultMythicSig = getitem.WeaponExtra[2];
                            }
                        }
                        weaponextraFields->DefaultDeadReplayEffectId = getitem.WeaponExtra[3];
                        weaponextraFields->DefaultKillEffectId = getitem.WeaponExtra[4];
                    }
                }
                for (auto asset : SkinSnapshot(weaponAssetGroupInstance))
                {
                    if (!asset)
                        continue;
                    weaponAssetFields = (WeaponAssetGroupFields *)((uintptr_t)asset + 0x40);
                    if (weaponAssetFields->Id == getitem.WeaponAsset[0])
                    {
                        if (getitem.WeaponAsset[1] / 10 == getitem.WeaponAsset[2] / 10 && getitem.WeaponAsset[1] > 0)
                        {
                            weaponAssetFields->FireEffectGroupID = getitem.WeaponAsset[1];
                        }
                    }
                }
                for (auto itemResource : SkinSnapshot(itemResourceConfigInstance))
                {
                    if (!itemResource)
                        continue;
                    itemFields = (ItemResourceFields *)((uintptr_t)itemResource + 0x10);
                    if (itemFields->ID == skinID)
                    {
                        itemFields->FxAssetID = getitem.WeaponConf[0];
                        itemFields->InventoryModelID = getitem.ItemResInt[1];
                        itemFields->ModelAssetIDRaw = getitem.ItemResInt[2];
                        itemFields->UIMiniSpriteNameIndex = getitem.ItemRes[0];
                        itemFields->UISmallSpriteNameIndex = getitem.ItemRes[1];
                        itemFields->UISpriteNameIndex = getitem.ItemRes[2];
                        itemFields->UISquareSpriteNameIndex = getitem.ItemRes[3];
                    }
                    if (itemFields->ID == skinID)
                    {
                        itemFields->FxAssetID = getitem.ItemResInt[0];
                        itemFields->InventoryModelID = getitem.ItemResInt[1];
                        itemFields->ModelAssetIDRaw = getitem.ItemResInt[2];
                        itemFields->UIMiniSpriteNameIndex = getitem.ItemRes[0];
                        itemFields->UISmallSpriteNameIndex = getitem.ItemRes[1];
                        itemFields->UISquareSpriteNameIndex = getitem.ItemRes[3];
                    }
                    if (itemFields->ID == itemID)
                    {
                        itemFields->FxAssetID = getitem.ItemResInt[0];
                        itemFields->InventoryModelID = getitem.ItemResInt[1];
                        itemFields->ModelAssetIDRaw = getitem.ItemResInt[2];
                        itemFields->UIMiniSpriteNameIndex = getitem.ItemRes[0];
                        itemFields->UISmallSpriteNameIndex = getitem.ItemRes[1];
                        itemFields->UISpriteNameIndex = getitem.ItemRes[2];
                    }
                    if (itemFields->ID == lootID)
                    {
                        itemFields->FxAssetID = getitem.ItemResInt[0];
                        itemFields->InventoryModelID = getitem.ItemResInt[1];
                        itemFields->ModelAssetIDRaw = getitem.ItemResInt[2];
                        itemFields->UIMiniSpriteNameIndex = getitem.ItemRes[0];
                        itemFields->UISmallSpriteNameIndex = getitem.ItemRes[1];
                        itemFields->UISpriteNameIndex = getitem.ItemRes[2];
                        itemFields->UISquareSpriteNameIndex = getitem.ItemRes[3];
                    }
                    if (itemFields->ID == extraID)
                    {
                        itemFields->FxAssetID = getitem.ItemResInt[0];
                        itemFields->InventoryModelID = getitem.ItemResInt[1];
                        itemFields->ModelAssetIDRaw = getitem.ItemResInt[2];
                        itemFields->UIMiniSpriteNameIndex = getitem.ItemRes[0];
                        itemFields->UISmallSpriteNameIndex = getitem.ItemRes[1];
                        itemFields->UISpriteNameIndex = getitem.ItemRes[2];
                    }
                    if (itemFields->ID == baseID)
                    {
                        itemFields->FxAssetID = getitem.ItemResInt[0];
                        itemFields->InventoryModelID = getitem.ItemResInt[1];
                        itemFields->ModelAssetIDRaw = getitem.ItemResInt[2];
                        itemFields->UIMiniSpriteNameIndex = getitem.ItemRes[0];
                        itemFields->UISmallSpriteNameIndex = getitem.ItemRes[1];
                        itemFields->UISpriteNameIndex = getitem.ItemRes[2];
                    }
                }

                int fireEffectID = 0;
                for (auto asset : SkinSnapshot(weaponAssetGroupInstance))
                {
                    if (!asset)
                        continue;
                    weaponAssetFields = (WeaponAssetGroupFields *)((uintptr_t)asset + 0x40);
                    if (weaponAssetFields && weaponAssetFields->Id == getitem.WeaponAsset[0])
                    {
                        fireEffectID = weaponAssetFields->FireEffectGroupID;
                        break;
                    }
                }

                if (fireEffectID > 0)
                {
                    for (auto fireConf : SkinSnapshot(weaponFireEffectInstance))
                    {
                        if (!fireConf)
                            continue;
                        WeaponFireEffectFields *wfFields = (WeaponFireEffectFields *)((uintptr_t)fireConf + 0x10);
                        if (!Tools::IsPtrValid(wfFields))
                            continue;

                        if (wfFields->Id == fireEffectID)
                        {
                            wfFields->LevelEffectDelayTimeUI = 0.1f;
                            wfFields->LevelEffectDelayTimeUI_King = 0.1f;
                            auto *killCountArray = *(Array<int> **)((uintptr_t)fireConf + 0x118);
                            if (killCountArray && Tools::IsPtrValid(killCountArray))
                            {
                                for (int i = 0; i < killCountArray->getLength(); ++i)
                                {
                                    killCountArray->m_Items[i] = 0;
                                }
                            }
                            auto *killCountArrayKing = *(Array<int> **)((uintptr_t)fireConf + 0x120);
                            if (killCountArrayKing && Tools::IsPtrValid(killCountArrayKing))
                            {
                                for (int i = 0; i < killCountArrayKing->getLength(); ++i)
                                {
                                    killCountArrayKing->m_Items[i] = 0;
                                }
                            }
                            break;
                        }
                    }
                }
            };

            for (const auto& getitem : itemData)
            {
                if (strlen(searchQuery) >= MIN_SEARCH_LENGTH) {
                    std::string nameLower = ToLower(getitem.itemName);
                    if (nameLower.find(searchLower) == std::string::npos)
                        continue;
                }
                if (sBool[getitem.itemName])
                {
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 8));
                    WeaponSkinCheckbox(getitem.itemName.c_str(), &sBool[getitem.itemName],
                                       [&, getitem]()
                    {
                        std::string currentBase = GetBaseWeaponName(getitem.itemName);
                        for (const auto& other : itemData)
                        {
                            if (other.itemName != getitem.itemName &&
                                    GetBaseWeaponName(other.itemName) == currentBase)
                            {
                                sBool[other.itemName] = false;
                            }
                        }
                        applyWeaponSkin(getitem);
                    });
                    ImGui::PopStyleVar();
                }
            }

            for (const auto& getitem : itemData)
            {
                if (strlen(searchQuery) >= MIN_SEARCH_LENGTH) {
                    std::string nameLower = ToLower(getitem.itemName);
                    if (nameLower.find(searchLower) == std::string::npos)
                        continue;
                }
                if (!sBool[getitem.itemName])
                {
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 8));
                    WeaponSkinCheckbox(getitem.itemName.c_str(), &sBool[getitem.itemName],
                                       [&, getitem]()
                    {
                        std::string currentBase = GetBaseWeaponName(getitem.itemName);
                        for (const auto& other : itemData)
                        {
                            if (other.itemName != getitem.itemName &&
                                    GetBaseWeaponName(other.itemName) == currentBase)
                            {
                                sBool[other.itemName] = false;
                            }
                        }
                        applyWeaponSkin(getitem);
                    });
                    ImGui::PopStyleVar();
                }
            }
        }
        else
        {
            ImGui::TextColored(ImVec4(1, 0.5f, 0.5f, 1), "No weapon skins loaded");
        }
        break;

    case 5:
        // --- Camo Manager Tab Content ---
        ImGui::TextColored(ImVec4(1, 0.8f, 0, 1), "Inject Camo to Mythic/Legendary Skins");
        ImGui::Dummy(ImVec2(0, 10));

        if (custom::Checkbox("Default / OFF", &camoOff)) {
            if (camoOff) {
                camoDiamond = false;
                camoRedSprite = false;
                camoGold = false;
                camoPlatinum = false;
                camoDamascus = false;
                camoAetherCrystal = false;
                camoGlacialRipple = false;
                for (const auto& getitem : itemData) {
                    if (getitem.itemName.find("[M]") != std::string::npos || getitem.itemName.find("[L]") != std::string::npos) {
                        for (auto conf : SkinSnapshot(weaponConfInstance)) {
                            if (!conf) continue;
                            weaponconfFields = (WeaponConfFields*)((uintptr_t)conf + 0x20);
                            if (weaponconfFields->ID == getitem.WeaponConf[2]) weaponconfFields->DefWeaponSkinID = 0;
                        }
                    }
                }
            }
        }

        ImGui::Separator();

        if (custom::Checkbox("Diamond Camo", &camoDiamond)) {
            if (camoDiamond) {
                camoOff = false;
                camoRedSprite = false;
                for (const auto& getitem : itemData) {
                    if (getitem.itemName.find("[M]") != std::string::npos || getitem.itemName.find("[L]") != std::string::npos) {
                        for (auto conf : SkinSnapshot(weaponConfInstance)) {
                            if (!conf) continue;
                            weaponconfFields = (WeaponConfFields*)((uintptr_t)conf + 0x20);
                            if (weaponconfFields->ID == getitem.WeaponConf[2]) {
                                weaponconfFields->DefWeaponSkinID = ID_DIAMOND; 
                            }
                        }
                    }
                }
            }
        }

        if (custom::Checkbox("Red Sprite Camo", &camoRedSprite)) {
            if (camoRedSprite) {
                camoOff = false;
                camoDiamond = false;
                for (const auto& getitem : itemData) {
                    if (getitem.itemName.find("[M]") != std::string::npos || getitem.itemName.find("[L]") != std::string::npos) {
                        for (auto conf : SkinSnapshot(weaponConfInstance)) {
                            if (!conf) continue;
                            weaponconfFields = (WeaponConfFields*)((uintptr_t)conf + 0x20);
                            if (weaponconfFields->ID == getitem.WeaponConf[2]) {
                                weaponconfFields->DefWeaponSkinID = ID_RED_SPRITE; 
                            }
                        }
                    }
                }
            }
        }
        
        if (custom::Checkbox("Gold Camo", &camoGold)) {
            if (camoGold) {
                camoOff = false;
                camoRedSprite = false;
                for (const auto& getitem : itemData) {
                    if (getitem.itemName.find("[M]") != std::string::npos || getitem.itemName.find("[L]") != std::string::npos) {
                        for (auto conf : SkinSnapshot(weaponConfInstance)) {
                            if (!conf) continue;
                            weaponconfFields = (WeaponConfFields*)((uintptr_t)conf + 0x20);
                            if (weaponconfFields->ID == getitem.WeaponConf[2]) {
                                weaponconfFields->DefWeaponSkinID = ID_GOLD; 
                            }
                        }
                    }
                }
            }
        }
        
        if (custom::Checkbox("Platimun Camo", &camoPlatinum)) {
            if (camoPlatinum) {
                camoOff = false;
                camoGold = false;
                for (const auto& getitem : itemData) {
                    if (getitem.itemName.find("[M]") != std::string::npos || getitem.itemName.find("[L]") != std::string::npos) {
                        for (auto conf : SkinSnapshot(weaponConfInstance)) {
                            if (!conf) continue;
                            weaponconfFields = (WeaponConfFields*)((uintptr_t)conf + 0x20);
                            if (weaponconfFields->ID == getitem.WeaponConf[2]) {
                                weaponconfFields->DefWeaponSkinID = ID_PLATINUM; 
                            }
                        }
                    }
                }
            }
        }
        
        if (custom::Checkbox("Damascus Camo", &camoDamascus)) {
            if (camoDamascus) {
                camoOff = false;
                camoPlatinum = false;
                for (const auto& getitem : itemData) {
                    if (getitem.itemName.find("[M]") != std::string::npos || getitem.itemName.find("[L]") != std::string::npos) {
                        for (auto conf : SkinSnapshot(weaponConfInstance)) {
                            if (!conf) continue;
                            weaponconfFields = (WeaponConfFields*)((uintptr_t)conf + 0x20);
                            if (weaponconfFields->ID == getitem.WeaponConf[2]) {
                                weaponconfFields->DefWeaponSkinID = ID_DAMASCUS; 
                            }
                        }
                    }
                }
            }
        }
        
        if (custom::Checkbox("Aether Crystal Camo", &camoAetherCrystal)) {
            if (camoAetherCrystal) {
                camoOff = false;
                camoDamascus = false;
                for (const auto& getitem : itemData) {
                    if (getitem.itemName.find("[M]") != std::string::npos || getitem.itemName.find("[L]") != std::string::npos) {
                        for (auto conf : SkinSnapshot(weaponConfInstance)) {
                            if (!conf) continue;
                            weaponconfFields = (WeaponConfFields*)((uintptr_t)conf + 0x20);
                            if (weaponconfFields->ID == getitem.WeaponConf[2]) {
                                weaponconfFields->DefWeaponSkinID = ID_AETHER_CRYSTAL; 
                            }
                        }
                    }
                }
            }
        }
           
        if (custom::Checkbox("Glacial Ripple Camo", &camoGlacialRipple)) {
            if (camoGlacialRipple) {
                camoOff = false;
                camoAetherCrystal = false;
                for (const auto& getitem : itemData) {
                    if (getitem.itemName.find("[M]") != std::string::npos || getitem.itemName.find("[L]") != std::string::npos) {
                        for (auto conf : SkinSnapshot(weaponConfInstance)) {
                            if (!conf) continue;
                            weaponconfFields = (WeaponConfFields*)((uintptr_t)conf + 0x20);
                            if (weaponconfFields->ID == getitem.WeaponConf[2]) {
                                weaponconfFields->DefWeaponSkinID = ID_GLACIAL_RIPPLE; 
                            }
                        }
                    }
                }
            }
        }
    break;
    case 6:
        if (!snowboardData.empty()) {
            // Selected (ON) first
            for (const auto& sb : snowboardData) {
                if (strlen(searchQuery) >= MIN_SEARCH_LENGTH &&
                    ToLower(sb.skinName).find(searchLower) == std::string::npos)
                    continue;
                if (sBool[sb.skinName]) {
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 8));
                    bool wasOn = sBool[sb.skinName];
                    DrawTieredSkinCheckbox(sb.skinName.c_str(), &sBool[sb.skinName], [&, sb]() {
                        SelectSnowboardSkin(sb.skinId);
                        for (const auto& other : snowboardData) {
                            if (other.skinName != sb.skinName)
                                sBool[other.skinName] = false;
                        }
                    });
                    if (wasOn && !sBool[sb.skinName]) {
                        activeVehicleSkins.erase(31707110);
                        activeVehicleSkinsById.clear();
                        activeVehicleSkinConfs.erase(sb.skinId);
                    }
                    ImGui::PopStyleVar();
                }
            }
            // Unselected (OFF) below
            for (const auto& sb : snowboardData) {
                if (strlen(searchQuery) >= MIN_SEARCH_LENGTH &&
                    ToLower(sb.skinName).find(searchLower) == std::string::npos)
                    continue;
                if (!sBool[sb.skinName]) {
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 8));
                    DrawTieredSkinCheckbox(sb.skinName.c_str(), &sBool[sb.skinName], [&, sb]() {
                        SelectSnowboardSkin(sb.skinId);
                        for (const auto& other : snowboardData) {
                            if (other.skinName != sb.skinName)
                                sBool[other.skinName] = false;
                        }
                    });
                    ImGui::PopStyleVar();
                }
            }
        } else {
            ImGui::TextColored(ImVec4(1, 0.5f, 0.5f, 1), "No snowboard skins loaded");
        }
        break;
    }
    ImGui::EndChild(); // ##SkinList

    if (showKeyboard) {
        if (activeInputID == "##SearchCustomChar") {
            RenderVirtualKeyboard("##VirtualKeyboard", charSearchQuery, IM_ARRAYSIZE(charSearchQuery), &showKeyboard);
        } else {
            RenderVirtualKeyboard("##VirtualKeyboard", searchQuery, IM_ARRAYSIZE(searchQuery), &showKeyboard);
        }
    }

    ImGui::EndChild(); // SkinContent
}

inline void RenderTab4Content() {
    RenderSkinCategoryContent(skinSubTab, true);
}
