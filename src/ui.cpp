/*!***************************************************
 * @file     ui.cpp
 * @brief    Handels all UI operations
 * @details  draws the menus, and canvas.
 * @note     
 * @date     2026.01.19
 * @author   bearded.griffin
 ****************************************************/

#include "ui.h"
#include "imgui.h"
#include "utils.h"
#include <string>
#include <cstring>
#include "printer.h"
#include "protocol.h"
#include "portable-file-dialogs.h"

namespace UI {

/*!***************************************************
 * @brief    Draw the main menu
 * @details  Draws the drop down main menu that
 * contains all the options for the program.
 * @param    project Project&
 * @return   void
 * @note     
 * @date     2026.01.19
 * @author   bearded.griffin
 ****************************************************/
void DrawMainMenu(Project &project) {
// 1. Declare a flag to track if we need to open the popup
static bool triggerScanPopup = false;

if (ImGui::BeginMainMenuBar()) {
  if (ImGui::BeginMenu("File")) {
    if (ImGui::MenuItem("Save Project"))
      Utils::SaveProject("project.flbl", project);
    if (ImGui::MenuItem("Load Project"))
      Utils::LoadProject("", project);

    ImGui::Separator();

    if (ImGui::MenuItem("Export to PNG (Test Print)")) {
      Utils::ExportProjectToPNG("test_label.png", project);
    }

    // Printer Menu
    if (ImGui::BeginMenu("Printer")) {
      if (Printer::Get().IsConnected()) {
        ImGui::TextColored(ImVec4(0, 1, 0, 1), "Connected: %s",
                           Printer::Get().GetConnectedName().c_str());
        if (ImGui::MenuItem("Disconnect")) {
          Printer::Get().Disconnect();
        }
      } else {
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "Status: Disconnected");
      }

      ImGui::Separator();

      if (ImGui::MenuItem("Scan for Devices")) {
        Printer::Get().StartScan();
        triggerScanPopup = true;
      }
      ImGui::EndMenu();
    }

    if (ImGui::MenuItem("Print Label")) {
          Protocol::PrintLabel(project);
      }

    ImGui::Separator();
    if (ImGui::MenuItem("Exit")) { /* TODO: flag to close */
    }
    ImGui::EndMenu();
  }
  ImGui::EndMainMenuBar();
}

// --- POPUP LOGIC ---

// 2. Handle the flag in the ROOT scope (matching BeginPopupModal)
if (triggerScanPopup) {
  ImGui::OpenPopup("DeviceScanPopup");
  triggerScanPopup = false;
}

static std::vector<BluetoothDevice> foundDevices;

// Now this ID matches the one opened above
if (ImGui::BeginPopupModal("DeviceScanPopup", NULL,
                           ImGuiWindowFlags_AlwaysAutoResize)) {

  if (Printer::Get().IsScanning()) {
    ImGui::Text("Scanning Bluetooth...");
    ImGui::Text("Please wait approx 10 seconds.");

    const char *spinner = "|/-\\";
    int frame = (int)(ImGui::GetTime() / 0.1f) % 4;
    ImGui::Text(" %c ", spinner[frame]);
  } else if (Printer::Get().HasScanResults()) {
    foundDevices = Printer::Get().GetScanResults();
  }

  if (!Printer::Get().IsScanning()) {
    ImGui::Text("Found Devices: %zu", foundDevices.size());
    ImGui::Separator();

    if (foundDevices.empty()) {
      ImGui::TextDisabled("No devices found.");
      ImGui::TextDisabled("(Check permissions: sudo setcap ...)");
    }

    for (const auto &dev : foundDevices) {
      std::string label = dev.name + " (" + dev.address + ")";
      if (ImGui::Button(label.c_str())) {
        Printer::Get().Connect(dev.address);
        ImGui::CloseCurrentPopup();
      }
    }

    ImGui::Separator();
    if (ImGui::Button("Close")) {
      ImGui::CloseCurrentPopup();
    }
  }

  ImGui::EndPopup();
}
}

/*!***************************************************
 * @brief    Draws the side bar
 * @details  It draws the side "Inspector" bar on the left side.
 * @param    project Project&
 * @param    selectedIndex int& 
 * @return   void
 * @note     
 * @date     2026.01.19
 * @author   bearded.griffin
 ****************************************************/
void DrawSidebar(Project &project, int &selectedIndex) {
  ImGui::Begin("Inspector", nullptr,
               ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);
  ImGui::SetWindowPos({0, 20}, ImGuiCond_FirstUseEver);
  ImGui::SetWindowSize({300, 600}, ImGuiCond_FirstUseEver);

  // --- 1. Project Settings ---
  ImGui::Text("Project Settings");
  ImGui::Separator();

  const char *current_item =
      Utils::LabelSizes[project.selectedLabelIndex].name.c_str();
  if (ImGui::BeginCombo("Canvas Size", current_item)) {
    for (int i = 0; i < Utils::LabelSizes.size(); i++) {
      bool is_selected = (project.selectedLabelIndex == i);
      if (ImGui::Selectable(Utils::LabelSizes[i].name.c_str(), is_selected)) {
        project.selectedLabelIndex = i;
      }
      if (is_selected)
        ImGui::SetItemDefaultFocus();
    }
    ImGui::EndCombo();
  }

  

  // The Checkboxes
  ImGui::Checkbox("Show Grid", &project.showGrid);
  ImGui::SameLine();
  ImGui::Checkbox("Dark Mode", &project.darkTheme);

  ImGui::Spacing();
  ImGui::Text("Objects Tree");
  ImGui::Separator();

  // --- 2. Object Tree ---
  for (int i = 0; i < project.objects.size(); i++) {
    LabelObject &obj = project.objects[i];

    // Generate a nice name like "QR: www.google.com"
    std::string typePrefix;
    switch (obj.type) {
    case ObjectType::Text:
      typePrefix = " [T] ";
      break;
    case ObjectType::QRCode:
      typePrefix = " [QR] ";
      break;
    case ObjectType::Image:
      typePrefix = " [IMG] ";
      break;
    case ObjectType::Field:
      typePrefix = " [FLD] ";
      break;
    }

    std::string displayName = typePrefix + obj.data;
    // Clamp long names for the UI
    if (displayName.length() > 25)
      displayName = displayName.substr(0, 22) + "...";

    // Unique ID for ImGui
    std::string id = displayName + "##" + std::to_string(i);

    if (ImGui::Selectable(id.c_str(), selectedIndex == i)) {
      selectedIndex = i;
    }
  }

  ImGui::Spacing();
  ImGui::Separator();

  // --- 3. Properties ---
  if (selectedIndex >= 0 && selectedIndex < project.objects.size()) {
    LabelObject &obj = project.objects[selectedIndex];
    ImGui::Text("Properties");
    ImGui::DragFloat("X", &obj.x);
    ImGui::DragFloat("Y", &obj.y);

    if (obj.type == ObjectType::Text || obj.type == ObjectType::Field) {
      // Text objects use Font Size
      ImGui::SliderFloat("Font Size", &obj.fontSize, 10.0f, 100.0f);
    } else if (obj.type == ObjectType::QRCode) {
      // QR Codes must be square -> Single "Size" control
      if (ImGui::DragFloat("Size", &obj.width, 1.0f, 10.0f, 500.0f)) {
        obj.height = obj.width; // Force square aspect ratio
      }
    } else if (obj.type == ObjectType::Image) {
      // Images can be rectangular -> Separate Width/Height controls
      ImGui::DragFloat("Width", &obj.width);
      ImGui::DragFloat("Height", &obj.height);

      // --- NEW: Image File Picker ---
      ImGui::Spacing();
      if (ImGui::Button("Browse Image...")) {
        auto selection =
            pfd::open_file("Select Image", ".",
                           {"Image Files", "*.png *.jpg *.jpeg *.bmp"})
                .result();
        if (!selection.empty()) {
          // 1. Update Path
          obj.data = selection[0];

          // 2. Unload old texture if exists
          if (obj.texture.id != 0)
            UnloadTexture(obj.texture);

          // 3. Load new texture
          // We load as Image first to resize it to a reasonable default if
          // needed
          Image img = LoadImage(obj.data.c_str());
          if (img.data != NULL) {
            // Auto-set width/height for new images
            if (obj.width == 0 || obj.height == 0) {
              obj.width = (float)img.width;
              obj.height = (float)img.height;
            }
            obj.texture = LoadTextureFromImage(img);
            UnloadImage(img);
          }
        }
      }
    }

    static char buffer[256];
    if (!ImGui::IsItemActive())
      strncpy(buffer, obj.data.c_str(), sizeof(buffer));

    // Label changes based on type
    const char *label = (obj.type == ObjectType::QRCode)  ? "Data"
                        : (obj.type == ObjectType::Image) ? "File Path"
                                                          : "Text";
    if (ImGui::InputText(label, buffer, sizeof(buffer))) {
      obj.data = buffer;
    }
  }

  ImGui::Spacing();
  ImGui::Separator();

  // --- 4. Add Buttons ---
  if (ImGui::Button("Add Text")) {
    project.objects.push_back(
        {ObjectType::Text, 50, 50, 0, 0, "New Text", 20.0f, 0x000000FF});
    selectedIndex = project.objects.size() - 1;
  }
  ImGui::SameLine();
  if (ImGui::Button("Add QR")) {
    project.objects.push_back({ObjectType::QRCode, 50, 50, 100, 100,
                               "www.example.com", 0, 0x000000FF});
    selectedIndex = project.objects.size() - 1;
  }
  ImGui::SameLine();
  if (ImGui::Button("Add Field")) {
    project.objects.push_back(
        {ObjectType::Field, 50, 50, 0, 0, "{ColumnName}", 20.0f, 0x000000FF});
    selectedIndex = project.objects.size() - 1;
  }

  ImGui::End();
}

}