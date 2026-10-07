#pragma once

#include "../../ImGui/imgui_settings.h"

#include <chrono>
#include <cctype>
#include <cstdio>
#include <ctime>
#include <string>
#include <cmath>

namespace font {
    extern ImFont* inter_semibold;
}

extern std::chrono::steady_clock::time_point appStartTime;

namespace floating_info {

// --- MATCH TRACKING VARIABLES ---
inline int totalMatches = 0;
inline bool wasInMatch = false;

inline std::string FormatTimeDuration(std::chrono::steady_clock::duration duration) {
    const auto hours = std::chrono::duration_cast<std::chrono::hours>(duration).count();
    const auto minutes = std::chrono::duration_cast<std::chrono::minutes>(duration % std::chrono::hours(1)).count();
    const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration % std::chrono::minutes(1)).count();
    char buffer[32] = {};
    std::snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d", (int)hours, (int)minutes, (int)seconds);
    return std::string(buffer);
}

// Helper: Animated Neon Color
inline ImColor GetNeonColor(float t, float speed = 1.0f, float offset = 0.0f) {
    float r = 0.5f + 0.5f * std::sin(t * speed + offset);
    float g = 0.5f + 0.5f * std::sin(t * speed + offset + 2.094f);
    float b = 0.5f + 0.5f * std::sin(t * speed + offset + 4.188f);
    return ImColor((int)(r * 255), (int)(g * 255), (int)(b * 255), 255);
}

inline void Render(ImDrawList* draw, float screenWidth, float screenHeight) {
    IM_UNUSED(screenWidth);
    if (draw == nullptr || Config.ExtraMenu.ClearDisplay) {
        return;
    }

    if (!font::inter_semibold) {
        return;
    }

    // --- MATCH TRACKING LOGIC ---
    bool isInMatch = false;
    auto* get_LocalPawn = GamePlay::get_LocalPawn();
    if (Tools::IsPtrValid(get_LocalPawn)) {
        isInMatch = true; 
    }

    if (isInMatch && !wasInMatch) {
        totalMatches++;
        wasInMatch = true;
    } else if (!isInMatch && wasInMatch) {
        wasInMatch = false;
    }

    // --- TIME & DATE CALCULATION ---
    const auto now = std::chrono::system_clock::now();
    std::time_t currentTime = std::chrono::system_clock::to_time_t(now);
    std::tm* localTime = std::localtime(&currentTime);

    char dateStr[64] = {};
    char timeStr[64] = {};
    std::strftime(dateStr, sizeof(dateStr), "%b %d, %Y", localTime); 
    std::strftime(timeStr, sizeof(timeStr), "%I:%M:%S %p", localTime); 

    for (char* p = dateStr; *p; ++p) *p = (char)std::toupper((unsigned char)*p);
    for (char* p = timeStr; *p; ++p) *p = (char)std::toupper((unsigned char)*p);

    auto currentDuration = std::chrono::steady_clock::now() - appStartTime;
    std::string playTime = FormatTimeDuration(currentDuration);

    char totalMatchesStr[32] = {};
    std::snprintf(totalMatchesStr, sizeof(totalMatchesStr), "%d", totalMatches);

    // --- ANIMATED VISUALS ---
    static float animTime = 0.0f;
    static float popIn = 0.0f;
    static float slideY = 0.0f;
    animTime += ImGui::GetIO().DeltaTime;

    // Smooth pop-in animation (slides up)
    if (popIn < 1.0f) {
        popIn += ImGui::GetIO().DeltaTime * 4.0f;
        if (popIn > 1.0f) popIn = 1.0f;
    }
    slideY = (1.0f - popIn) * 30.0f; // Slide up from bottom

    // --- PREMIUM BOX RENDER ---
    const float boxW = 245.0f * c::scale;
    const float boxH = 152.0f * c::scale; 
    const float padding = 24.0f * c::scale;
    
    const float boxX = padding;
    const float boxY = (screenHeight - padding - boxH) + slideY; // Add slideY here
    
    const float lineSpacing = 19.0f * c::scale;
    const float textStartX = boxX + 18.0f * c::scale;
    const float textEndX = boxX + boxW - 18.0f * c::scale; 
    const float startY = boxY + 16.0f * c::scale;

    // --- CYBERPUNK BACKGROUND EFFECTS ---
    // 1. Ambient Glow
    draw->AddRectFilled(ImVec2(boxX - 4.0f, boxY - 4.0f), ImVec2(boxX + boxW + 4.0f, boxY + boxH + 4.0f), ImColor((int)(menu[0] * 255), (int)(menu[1] * 255), (int)(menu[2] * 255), 15), 12.0f * c::scale);
    
    // 2. Main Background (Dark Cyberpunk)
    draw->AddRectFilled(ImVec2(boxX, boxY), ImVec2(boxX + boxW, boxY + boxH), ImColor(8, 8, 8, 245), 10.0f * c::scale);

    // 3. Moving "Scanning" Horizontal Lines
    static float scanY = 0.0f;
    scanY += ImGui::GetIO().DeltaTime * 80.0f;
    if (scanY > boxH) scanY = 0.0f;
    draw->AddLine(ImVec2(boxX, boxY + scanY), ImVec2(boxX + boxW, boxY + scanY), ImColor((int)(menu[0] * 255), (int)(menu[1] * 255), (int)(menu[2] * 255), 20), 1.0f);

    // 4. Floating Background Particles
    for (int i = 0; i < 12; ++i) {
        float pX = boxX + (std::sin(animTime * 0.5f + i * 1.3f) * 0.5f + 0.5f) * boxW;
        float pY = boxY + (std::cos(animTime * 0.7f + i * 2.1f) * 0.5f + 0.5f) * boxH;
        draw->AddCircleFilled(ImVec2(pX, pY), 1.0f, ImColor((int)(menu[0] * 255), (int)(menu[1] * 255), (int)(menu[2] * 255), 40));
    }

    ImColor borderColor((int)(menu[0] * 255), (int)(menu[1] * 255), (int)(menu[2] * 255), 255);
    draw->AddRect(ImVec2(boxX, boxY), ImVec2(boxX + boxW, boxY + boxH), borderColor, 10.0f * c::scale, 0, 1.8f * c::scale);
    // FIXED: Convert alpha to float to avoid ambiguous constructor call
    draw->AddRect(ImVec2(boxX - 2.0f, boxY - 2.0f), ImVec2(boxX + boxW + 2.0f, boxY + boxH + 2.0f), ImColor(borderColor.Value.x, borderColor.Value.y, borderColor.Value.z, 60.0f / 255.0f), 12.0f * c::scale, 0, 1.0f);

    // --- TEXT RENDERING ---
    auto drawTextWithSpacing = [&](const char* text, ImVec2 pos, ImColor color, float customTracking = 1.1f, bool rightAlign = false) {
        float xOffset = 0.0f;
        const float charSpacing = customTracking * c::scale;
        float fontSize = 11.5f * c::scale;

        if (rightAlign) {
            float totalWidth = 0.0f;
            for (int i = 0; text[i] != '\0'; ++i) {
                char glyph[2] = { text[i], '\0' };
                totalWidth += font::inter_semibold->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, glyph).x + charSpacing;
            }
            pos.x -= totalWidth;
        }

        for (int i = 0; text[i] != '\0'; ++i) {
            char glyph[2] = { text[i], '\0' };
            draw->AddText(font::inter_semibold, fontSize, ImVec2(pos.x + xOffset, pos.y), color, glyph);
            xOffset += font::inter_semibold->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, glyph).x + charSpacing;
        }
    };

    // --- DRAWING THE INTERFACE ---
    ImColor neonPurple((int)(menu[0] * 255), (int)(menu[1] * 255), (int)(menu[2] * 255), 255);
    ImColor neonCyan((int)(menu[0] * 255), (int)(menu[1] * 255), (int)(menu[2] * 255), 255);
    ImColor neonPink(142, 142, 148, 255);
    ImColor pureWhite(235, 235, 235, 255);

    // 1. Header (Neon Purple)
    drawTextWithSpacing("ETHNIR NOIR", ImVec2(textStartX, startY), neonPurple, 1.4f);
    
    // Status Dot + DELUXE (Neon Cyan)
    float dotRadius = 3.0f * c::scale;
    float deluxeTextWidth = font::inter_semibold->CalcTextSizeA(11.5f * c::scale, FLT_MAX, 0.0f, "DELUXE").x + (1.4f * 6);
    ImVec2 dotPos = ImVec2(textEndX - deluxeTextWidth - (8.0f * c::scale), startY + (6.0f * c::scale));
    
    draw->AddCircleFilled(dotPos, dotRadius, neonCyan);
    draw->AddCircle(dotPos, dotRadius + (1.5f * c::scale), ImColor((int)(menu[0] * 255), (int)(menu[1] * 255), (int)(menu[2] * 255), 50), 0, 1.0f);

    drawTextWithSpacing("DELUXE", ImVec2(textEndX, startY), neonCyan, 1.4f, true);
    
    // Divider (Animated)
    float dividerY = startY + 18.0f * c::scale;
    draw->AddLine(ImVec2(textStartX, dividerY), ImVec2(textEndX, dividerY), borderColor, 1.0f);

    float currentY = dividerY + 10.0f * c::scale;

    // 2. Info Rows (Text in Pure White, Labels in Neon Pink)
    drawTextWithSpacing("DATE", ImVec2(textStartX, currentY), neonPink, 1.0f);
    drawTextWithSpacing(dateStr, ImVec2(textEndX, currentY), pureWhite, 1.1f, true);
    
    currentY += lineSpacing;
    drawTextWithSpacing("TIME", ImVec2(textStartX, currentY), neonPink, 1.0f);
    drawTextWithSpacing(timeStr, ImVec2(textEndX, currentY), pureWhite, 1.1f, true);
    
    currentY += lineSpacing;
    drawTextWithSpacing("SESSION", ImVec2(textStartX, currentY), neonPink, 1.0f); 
    drawTextWithSpacing(playTime.c_str(), ImVec2(textEndX, currentY), neonCyan, 1.1f, true);

    // Matches
    currentY += lineSpacing;
    drawTextWithSpacing("MATCHES", ImVec2(textStartX, currentY), neonPink, 1.0f);
    drawTextWithSpacing(totalMatchesStr, ImVec2(textEndX, currentY), pureWhite, 1.1f, true);

    // 3. Footer (Glitch Text)
    currentY += lineSpacing + 8.0f * c::scale;
    
    draw->AddLine(ImVec2(textStartX, currentY - 3.0f * c::scale), ImVec2(textStartX + 20.0f * c::scale, currentY - 3.0f * c::scale), borderColor, 2.0f);
    drawTextWithSpacing("ETHNIR NOIR", ImVec2(textStartX, currentY), neonPurple, 2.0f);
}

} // namespace floating_info