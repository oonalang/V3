#ifndef SEARCH_FUNCTION_H
#define SEARCH_FUNCTION_H

#include "../ImGui/imgui.h"
#include "../ImGui/imgui_internal.h"

// Make menu array accessible (assuming it's a global variable)
// If menu is not global, you'll need to pass it as another parameter
extern float menu[4];

inline void RenderSearchFunction(ImDrawList* draw) {
    static int foundCount = -1;
    
    ImVec2 buttonSize = ImVec2(280, 40);
    ImGui::SetCursorPosX((315 - buttonSize.x) / 2);
    
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(menu[0], menu[1], menu[2], 0.8f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(menu[0], menu[1], menu[2], 0.9f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(menu[0], menu[1], menu[2], 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImColor(255, 255, 255, 180).Value);
    ImGui::PushStyleColor(ImGuiCol_Text, ImColor(255, 255, 255, 255).Value);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 10.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2.0f);
    
    if (ImGui::Button("Search 67109633", buttonSize)) {
        kFox::ClearResult();
        kFox::SetSearchRange(RegionType::ALL);
        kFox::MemorySearch((char *)"67109633", Type::TYPE_DWORD);
        
        Maps_t* result = kFox::GetResult();
        foundCount = 0;
        while (result != nullptr) {
            foundCount++;
            result = result->next;
        }
    }
    
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(5);
    
    if (foundCount != -1) {
        ImGui::Spacing();
        ImGui::SetCursorPosX((315 - ImGui::CalcTextSize("Results found: 0000").x) / 2);
        ImGui::Text("Results found: %d", foundCount);
    }
}

#endif // SEARCH_FUNCTION_H
