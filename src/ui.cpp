/*!***************************************************
 * @file     ui.cpp
 * @brief    Handels all UI operations
 * @details  Draws the menus, sidebar, and popups.
 * @date     2026.01.23
 * @author   bearded.griffin
 ****************************************************/

#include "ui.h"
#include "imgui.h"
#include "portable-file-dialogs.h"
#include "printer.h"
#include "protocol.h"
#include "utils.h"

#include <chrono>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

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

  // Flags to trigger popups from the main menu scope
  static bool triggerScanPopup = false;
  static bool triggerBatchPopup = false;

  if (ImGui::BeginMainMenuBar()) {

    // --- FILE MENU ---
    if (ImGui::BeginMenu("File")) {
      if (ImGui::MenuItem("Save Project"))
        Utils::SaveProject("project.flbl", project);
      if (ImGui::MenuItem("Load Project"))
        Utils::LoadProject("", project);

      ImGui::Separator();

      if (ImGui::MenuItem("Export to PNG (Test Print)")) {
        Utils::ExportProjectToPNG("test_label.png", project);
      }

      ImGui::Separator();

      // --- CSV / BATCH PRINTING ---
      if (ImGui::MenuItem("Load CSV Data...")) {
        auto selection =
            pfd::open_file("Select CSV", ".", {"CSV Files", "*.csv"}).result();
        if (!selection.empty()) {
          Utils::LoadCSV(selection[0], project);

          project.currentCSVRow = 0;
          Utils::ApplyCSVDataToObjects(project);
        }
      }

      if (ImGui::MenuItem("Batch Print (CSV)")) {
        if (project.csvRows.empty()) {
          // Could add a toast/error here, but for now we just don't open
          std::cout << "[UI] No CSV loaded. Cannot batch print." << std::endl;
        } else {
          triggerBatchPopup = true;
        }
      }

      ImGui::Separator();
      if (ImGui::MenuItem("Exit")) { /* TODO: Close Window Flag */
      }
      ImGui::EndMenu();
    }

    // --- PRINTER MENU ---
    if (ImGui::BeginMenu("Printer")) {
      // Status Indicator
      if (Printer::Get().IsConnected()) {
        ImGui::TextColored(ImVec4(0, 1, 0, 1), "Connected: %s",
                           Printer::Get().GetConnectedName().c_str());
        if (ImGui::MenuItem("Disconnect")) {
          Printer::Get().Disconnect();
        }

        ImGui::Separator();

        // Direct Print Action
        if (ImGui::MenuItem("Print Single Label")) {
          Protocol::PrintLabel(project);
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

    ImGui::EndMainMenuBar();
  }

  // =========================================================
  // POPUP HANDLERS (Must be outside Menu Bar Scope)
  // =========================================================

  // --- 1. DEVICE SCAN POPUP ---
  if (triggerScanPopup) {
    ImGui::OpenPopup("DeviceScanPopup");
    triggerScanPopup = false;
  }

  static std::vector<BluetoothDevice> foundDevices;
  if (ImGui::BeginPopupModal("DeviceScanPopup", NULL,
                             ImGuiWindowFlags_AlwaysAutoResize)) {

    if (Printer::Get().IsScanning()) {
      ImGui::Text("Scanning Bluetooth...");
      ImGui::Text("Please wait approx 10 seconds.");

      // Simple Spinner
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

  // --- 2. BATCH PRINT POPUP ---
  if (triggerBatchPopup) {
    ImGui::OpenPopup("BatchPrintPopup");
    triggerBatchPopup = false;
  }

  if (ImGui::BeginPopupModal("BatchPrintPopup", NULL,
                             ImGuiWindowFlags_AlwaysAutoResize)) {
    static int startRow = 1;
    static int endRow = 0;

    // Initialize endRow once when opening
    if (endRow == 0 && !project.csvRows.empty())
      endRow = (int)project.csvRows.size();

    ImGui::Text("CSV File: %s", project.csvFilePath.c_str());
    ImGui::Text("Total Rows: %zu", project.csvRows.size());

    ImGui::Separator();
    ImGui::InputInt("Start Row", &startRow);
    ImGui::InputInt("End Row", &endRow);

    // Safety Clamps
    if (startRow < 1)
      startRow = 1;
    if (endRow > (int)project.csvRows.size())
      endRow = (int)project.csvRows.size();
    if (endRow < startRow)
      endRow = startRow;

    ImGui::Separator();

    if (ImGui::Button("PRINT BATCH")) {
      // --- THE BATCH LOOP ---
      for (int i = startRow - 1; i < endRow; i++) {
        // 1. Get Data
        std::vector<std::string> &rowData = project.csvRows[i];

        // 2. Create Temp Project
        Project tempProject = project;

        // 3. Inject Data
        for (auto &obj : tempProject.objects) {
          if (!obj.linkedColumn.empty()) {
            for (size_t c = 0; c < project.csvHeaders.size(); c++) {
              if (project.csvHeaders[c] == obj.linkedColumn) {
                if (c < rowData.size()) {
                  obj.data = rowData[c];
                }
                break;
              }
            }
          }
        }

        // 4. Print
        std::cout << "[Batch] Printing Row " << (i + 1) << "..." << std::endl;
        Protocol::PrintLabel(tempProject);

        // 5. Delay (Prevent buffer overflow)
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
      }
      ImGui::CloseCurrentPopup();
    }

    ImGui::SameLine();
    if (ImGui::Button("Cancel"))
      ImGui::CloseCurrentPopup();

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

  ImGui::Checkbox("Show Grid", &project.showGrid);
  ImGui::SameLine();
  ImGui::Checkbox("Dark Mode", &project.darkTheme);

  if (!project.csvRows.empty()) {
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::TextColored(ImVec4(0, 1, 1, 1), "Data Source");

    // File Name (Shortened)
    std::string filename = project.csvFilePath;
    size_t lastSlash = filename.find_last_of("/\\");
    if (lastSlash != std::string::npos)
      filename = filename.substr(lastSlash + 1);
    ImGui::Text("File: %s", filename.c_str());

    // Navigation Controls
    ImGui::Spacing();
    if (ImGui::Button("<<")) {
      project.currentCSVRow--;
      Utils::ApplyCSVDataToObjects(project);
    }

    ImGui::SameLine();
    // Display "Row 1 of 50" (Human readable 1-based index)
    ImGui::Text(" Row %d of %zu ", project.currentCSVRow + 1,
                project.csvRows.size());

    ImGui::SameLine();
    if (ImGui::Button(">>")) {
      project.currentCSVRow++;
      Utils::ApplyCSVDataToObjects(project);
    }

    // Manual Entry (Jump to Row)
    int tempRow = project.currentCSVRow + 1;
    if (ImGui::SliderInt("##RowSlider", &tempRow, 1,
                         (int)project.csvRows.size())) {
      project.currentCSVRow = tempRow - 1;
      Utils::ApplyCSVDataToObjects(project);
    }
  }

  ImGui::Spacing();
  ImGui::Text("Objects Tree");
  ImGui::Separator();

  // --- 2. Object Tree ---
  for (int i = 0; i < project.objects.size(); i++) {
    LabelObject &obj = project.objects[i];

    // Generate display name
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
    if (displayName.length() > 25)
      displayName = displayName.substr(0, 22) + "...";

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

    // Type-Specific Properties
    if (obj.type == ObjectType::Text || obj.type == ObjectType::Field) {
      ImGui::SliderFloat("Font Size", &obj.fontSize, 10.0f, 100.0f);
    } else if (obj.type == ObjectType::QRCode) {
      if (ImGui::DragFloat("Size", &obj.width, 1.0f, 10.0f, 500.0f)) {
        obj.height = obj.width; // Keep Square
      }
    } else if (obj.type == ObjectType::Image) {
      ImGui::DragFloat("Width", &obj.width);
      ImGui::DragFloat("Height", &obj.height);

      ImGui::Spacing();
      if (ImGui::Button("Browse Image...")) {
        auto selection =
            pfd::open_file("Select Image", ".",
                           {"Image Files", "*.png *.jpg *.jpeg *.bmp"})
                .result();
        if (!selection.empty()) {
          obj.data = selection[0];

          // Unload old
          if (obj.texture.id != 0)
            UnloadTexture(obj.texture);

          // Load new
          Image img = LoadImage(obj.data.c_str());
          if (img.data != NULL) {
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

    // --- DATA BINDING (CSV) ---
    // Allow binding for Text, Field, and QR Codes
    if (!project.csvHeaders.empty() &&
        (obj.type == ObjectType::Text || obj.type == ObjectType::Field ||
         obj.type == ObjectType::QRCode)) {

      ImGui::Spacing();
      ImGui::Separator();
      ImGui::Text("Data Binding (Batch)");

      std::string currentLink =
          obj.linkedColumn.empty() ? "[None]" : obj.linkedColumn;

      if (ImGui::BeginCombo("Link Column", currentLink.c_str())) {
        if (ImGui::Selectable("[None]", obj.linkedColumn.empty())) {
          obj.linkedColumn = "";
        }

        for (const auto &header : project.csvHeaders) {
          bool isSelected = (obj.linkedColumn == header);
          if (ImGui::Selectable(header.c_str(), isSelected)) {
            obj.linkedColumn = header;
            // Preview first row immediately
            if (!project.csvRows.empty()) {
              for (size_t i = 0; i < project.csvHeaders.size(); i++) {
                if (project.csvHeaders[i] == header) {
                  obj.data = project.csvRows[0][i];
                  break;
                }
              }
            }
          }
          if (isSelected)
            ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
      }
    }

    ImGui::Separator();

    // Manual Data Entry
    static char buffer[256];
    if (!ImGui::IsItemActive())
      strncpy(buffer, obj.data.c_str(), sizeof(buffer));

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
        {ObjectType::Text, 50, 50, 0, 0, "New Text", "", 20.0f, 0x000000FF});
    selectedIndex = project.objects.size() - 1;
  }
  ImGui::SameLine();
  if (ImGui::Button("Add QR")) {
    project.objects.push_back({ObjectType::QRCode, 50, 50, 100, 100,
                               "www.example.com", "", 0, 0x000000FF});
    selectedIndex = project.objects.size() - 1;
  }
  ImGui::SameLine();

  // IMAGE BUTTON
  if (ImGui::Button("Add Image")) {
    project.objects.push_back(
        {ObjectType::Image, 50, 50, 100, 100, "", "", 0, 0xFFFFFFFF});
    selectedIndex = project.objects.size() - 1;
  }

  ImGui::SameLine();
  if (ImGui::Button("Add Field")) {
    project.objects.push_back(
        {ObjectType::Field, 50, 50, 0, 0, "{Column}", "", 20.0f, 0x000000FF});
    selectedIndex = project.objects.size() - 1;
  }

  ImGui::End();
}

} // namespace UI