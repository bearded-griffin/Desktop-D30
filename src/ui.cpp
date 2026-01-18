#include "ui.h"
#include "imgui.h"
#include "utils.h"
#include <string>
#include <cstring> // Needed for strncpy

namespace UI {

    void DrawMainMenu(Project& project) {
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Save Project")) Utils::SaveProject("project.json", project);
                if (ImGui::MenuItem("Load Project")) Utils::LoadProject("project.json", project);
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }
    }

    void DrawSidebar(Project& project, int& selectedIndex) {
        ImGui::Begin("Inspector", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);
        ImGui::SetWindowPos({0, 20}, ImGuiCond_FirstUseEver);
        ImGui::SetWindowSize({300, 600}, ImGuiCond_FirstUseEver);

        // --- Label Settings ---
        ImGui::Text("Label Settings");
        ImGui::Separator();
        
        const char* current_item = Utils::LabelSizes[project.selectedLabelIndex].name.c_str();
        // FIX 1: Changed "Size" to "Canvas Size" to avoid ID collision
        if (ImGui::BeginCombo("Canvas Size", current_item)) {
            for (int i = 0; i < Utils::LabelSizes.size(); i++) {
                bool is_selected = (project.selectedLabelIndex == i);
                if (ImGui::Selectable(Utils::LabelSizes[i].name.c_str(), is_selected)) {
                    project.selectedLabelIndex = i;
                }
                if (is_selected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        ImGui::Spacing();
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
        
        if (selectedIndex >= 0 && selectedIndex < project.objects.size()) {
            LabelObject& obj = project.objects[selectedIndex];
            ImGui::Text("Properties");
            ImGui::DragFloat("X", &obj.x);
            ImGui::DragFloat("Y", &obj.y);
            
            // FIX 2: Changed "Size" to "Font Size"
            ImGui::SliderFloat("Font Size", &obj.fontSize, 10.0f, 100.0f);
            
            static char buffer[256];
            // We use a simplified logic here:
            // Only update the buffer from the source string if the text input is NOT active.
            // This prevents the buffer from being overwritten while you are typing.
            if (!ImGui::IsItemActive()) {
                 strncpy(buffer, obj.text.c_str(), sizeof(buffer));
            }
            
            if (ImGui::InputText("Text", buffer, sizeof(buffer))) {
                obj.text = buffer;
            }
        }

        ImGui::Spacing();
        // Just "Add Object" is fine, buttons usually have unique labels naturally
        if (ImGui::Button("Add Object", { -1, 0 })) { 
            project.objects.push_back({ 50.0f, 50.0f, "New Item", 20.0f, 0x000000FF });
            selectedIndex = project.objects.size() - 1;
        }

        ImGui::End();
    }
}