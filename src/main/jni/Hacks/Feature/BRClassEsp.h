#pragma once

#include <cstdint>
#include "Includes/SafePatch.h"
#include <string>
#include <unordered_map>
#include <vector>

namespace BRClassEspCfg
{
    constexpr uintptr_t ConfigCtorRva = 0xAB55EC8;
    constexpr uintptr_t GetNameRva = 0xAB56A60;
    constexpr uintptr_t PrevProfessionChipID = 0x1EC;
    constexpr uintptr_t ChipSkinID = 0x3F0;
    constexpr uintptr_t ConfigItemID = 0x24;
    constexpr float MaxDist = 120.0f;
}

struct BRClassDisplayData
{
    std::string name;
};

inline std::vector<void*>& BRClassConfigInstances()
{
    static std::vector<void*> instances;
    return instances;
}

inline void (*orig_BRProfessionChipConfig_ctor)(void* instance);

inline void hook_BRProfessionChipConfig_ctor(void* instance)
{
    if (orig_BRProfessionChipConfig_ctor != nullptr)
        orig_BRProfessionChipConfig_ctor(instance);

    if (!Tools::IsPtrValid(instance))
        return;

    std::vector<void*>& instances = BRClassConfigInstances();
    for (void* existing : instances) {
        if (existing == instance)
            return;
    }
    instances.push_back(instance);
}

inline void InstallBRClassEspConfigHook()
{
    static bool installed = false;
    if (installed)
        return;

    // Readable is not enough: Dobby writes a branch over the first bytes, so the
    // target has to be proven code (aligned + executable) and the trampoline has
    // to actually land. SafeDobbyHookAddr does both and logs the outcome.
    void* target = (void*)getAbsoluteAddress("libunity.so", BRClassEspCfg::ConfigCtorRva);
    if (!SafeDobbyHookAddr(target, (void*)hook_BRProfessionChipConfig_ctor,
                           (void**)&orig_BRProfessionChipConfig_ctor,
                           "BRProfessionChipConfig::.ctor"))
        return;

    installed = true;
}

inline int GetPlayerBRClassId(Pawn* pawn)
{
    if (!Tools::IsPtrValid(pawn))
        return 0;

    uintptr_t playerInfo = *(uintptr_t*)((uintptr_t)pawn + Class_Pawn_m_PlayerInfo);
    if (!Tools::IsPtrValid((void*)playerInfo))
        return 0;

    int classId = *(int*)(playerInfo + Class_PlayerInfo_m_ProfessionChipID);
    if (classId <= 0)
        classId = *(int*)(playerInfo + BRClassEspCfg::PrevProfessionChipID);
    return classId;
}

inline uint32_t GetPlayerBRClassSkinId(Pawn* pawn)
{
    if (!Tools::IsPtrValid(pawn))
        return 0;

    uintptr_t playerInfo = *(uintptr_t*)((uintptr_t)pawn + Class_Pawn_m_PlayerInfo);
    if (!Tools::IsPtrValid((void*)playerInfo))
        return 0;

    return *(uint32_t*)(playerInfo + BRClassEspCfg::ChipSkinID);
}

inline BRClassDisplayData GetBRClassDisplayData(Pawn* pawn)
{
    const int classId = GetPlayerBRClassId(pawn);
    if (classId > 0) {
        static std::unordered_map<int, BRClassDisplayData> dataCache;
        auto cached = dataCache.find(classId);
        if (cached != dataCache.end())
            return cached->second;

        for (void* conf : BRClassConfigInstances()) {
            if (!Tools::IsPtrValid(conf))
                continue;

            int itemId = *(int*)((uintptr_t)conf + BRClassEspCfg::ConfigItemID);
            if (itemId != classId)
                continue;

            static auto getName = reinterpret_cast<String* (*)(void*)>(
                getAbsoluteAddress("libunity.so", BRClassEspCfg::GetNameRva));
            if (getName == nullptr)
                break;

            String* name = getName(conf);
            if (!Tools::IsPtrValid(name) || name->getLength() <= 0)
                break;

            std::string text = name->CString();
            if (!text.empty()) {
                BRClassDisplayData data{text};
                dataCache[classId] = data;
                return data;
            }
            break;
        }

        return {"BR " + std::to_string(classId)};
    }

    const uint32_t skinId = GetPlayerBRClassSkinId(pawn);
    if (skinId > 0)
        return {"BR " + std::to_string(skinId)};

    return {""};
}

inline float BRClassEspScale(float distanceToMe)
{
    return ImClamp(1.12f - ((distanceToMe - 10.0f) * 0.0045f), 0.86f, 1.16f);
}

inline void DrawBRClassEsp(ImDrawList* draw, Pawn* pawn, float centerX, float headY, float boxWidth, float distanceToMe)
{
    (void)boxWidth;

    if (!Config.ESPMenu.BRClass || !Tools::IsPtrValid(pawn) || distanceToMe > BRClassEspCfg::MaxDist)
        return;

    BRClassDisplayData data = GetBRClassDisplayData(pawn);
    if (data.name.empty())
        return;

    const float scale = BRClassEspScale(distanceToMe);
    const float fontSize = 16.5f * scale;
    const float tagH = 25.0f * scale;
    const float padX = 11.0f * scale;
    const float headerH = 22.0f * scale;
    const float healthH = Config.ESPMenu.Health && Config.ESPMenu.HealthPosition == EspHealthPosition::HealthTop ? 5.0f * scale : 0.0f;
    const float gap = 3.0f * scale;
    const bool hasHeader = Config.ESPMenu.Name || Config.ESPMenu.Distance || (Config.ESPMenu.Health && Config.ESPMenu.HealthPosition == EspHealthPosition::HealthTop);

    ImVec2 textSize = ImGui::CalcTextSize(data.name.c_str());
    textSize.x *= fontSize / ImGui::GetFontSize();
    textSize.y *= fontSize / ImGui::GetFontSize();

    const float minW = 68.0f * scale;
    const float tagW = ImMax(textSize.x + padX * 2.0f, minW);
    const float x = centerX - tagW * 0.5f;
    const float headerStack = headerH + (healthH > 0.0f ? gap + healthH : 0.0f);
    const float y = headY - (hasHeader ? headerStack + tagH + 18.0f * scale : tagH + 12.0f * scale);

    draw->AddRectFilled(ImVec2(x, y), ImVec2(x + tagW, y + tagH), IM_COL32(0, 0, 0, 128), 3.0f * scale);
    draw->AddRectFilled(ImVec2(x, y), ImVec2(x + 2.0f * scale, y + tagH), IM_COL32(255, 214, 80, 210), 1.5f * scale);

    float textX = x + (tagW - textSize.x) * 0.5f;
    float textY = y + (tagH - textSize.y) * 0.5f - 1.0f * scale;
    DrawTextWithBorder1(draw, data.name, Vector2(textX, textY), IM_COL32(235, 238, 242, 255), IM_COL32(0, 0, 0, 255), fontSize);
}