#include "ui.h"
#include "imgui.h"
#include "utils.h"
#include <string>

namespace UI {

    void DrawMainMenu(Project& project) {
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Save Project")) {
                    Utils::SaveProject("project.json", project);
                }
                if (ImGui::MenuItem("Load Project")) {
                    Utils::LoadProject("project.json", project);
                }
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }
    }

    void DrawSidebar(Project& project, int& selectedIndex) {
        ImGui::Begin("Inspector", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);
        
        // Setup simple window positioning
        ImGui::SetWindowPos({0, 20}, ImGuiCond_FirstUseEver);
        ImGui::SetWindowSize({300, 600}, ImGuiCond_FirstUseEver);

        ImGui::Text("Objects List");
        ImGui::Separator();
        
        for (int i = 0; i < project.objects.size(); i++) {
            std::string label = "Object " + std::to_string(i);
            if (ImGui::Selectable(label.c_str(), selectedIndex == i)) {
                selectedIndex = i;
            }
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Text("Properties");

        if (selectedIndex >= 0 && selectedIndex < project.objects.size()) {
            LabelObject& obj = project.objects[selectedIndex];
            
            // FIX: Access .x and .y directly
            ImGui::DragFloat("X", &obj.x);
            ImGui::DragFloat("Y", &obj.y);
            ImGui::SliderFloat("Size", &obj.fontSize, 10.0f, 100.0f);
            
            ImGui::Text("Text: %s", obj.text.c_str());
        }

        if (ImGui::Button("Add Object")) {
            // FIX: Initializer list must match new struct: { x, y, text, fontSize, colorHex }
            // 0x000000FF is Black in Raylib hex format (R G B A)
            project.objects.push_back({ 100.0f, 100.0f, "New Item", 20.0f, 0x000000FF });
        }

        ImGui::End();
    }
}