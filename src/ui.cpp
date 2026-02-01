/*!***************************************************
 * @file     ui.cpp
 * @brief    Handels all UI operations
 * @details  Draws the menus, sidebar, and popups.
 * @date     2026.01.23
 * @author   bearded.griffin
 ****************************************************/

#include "ui.h"
#include "assets.h"
#include "imgui.h"
#include "printer.h"
#include "protocol.h"
#include "types.h"
#include "utils.h"

#include "portable-file-dialogs.h"
#include <chrono>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

namespace UI {

// Gloabal var for popup dialogs
static bool triggerIconPopup = false;
static bool triggerBorderPopup = false;
static bool triggerScanPopup = false;
static bool triggerBatchPopup = false;
static bool triggerLoadConfirmation = false;

// Exit State Flags
static bool exitRequested = false;
static bool forceQuit = false;

void RequestExit() { exitRequested = true; }
bool ShouldClose() { return forceQuit; }
void ClearExitRequest() { exitRequested = false; }

void DrawExitConfirmation(Project &project) {
  if (exitRequested) {
    if (!project.isDirty) {
      forceQuit = true;
    } else {
      ImGui::OpenPopup("ConfirmExit");
    }
  }

  if (ImGui::BeginPopupModal("ConfirmExit", NULL,
                             ImGuiWindowFlags_AlwaysAutoResize)) {
    ImGui::Text("You have unsaved changes!\n\n");
    ImGui::Separator();

    if (ImGui::Button("Save and Exit", ImVec2(120, 0))) {
      bool saveSuccess = false;
      if (project.csvFilePath.empty()) {
        saveSuccess = Utils::SaveProject(project);
      } else {
        saveSuccess = Utils::SaveProject(project, project.csvFilePath);
      }

      if (saveSuccess) {
        project.isDirty = false;
        forceQuit = true;
      }
      ImGui::CloseCurrentPopup();
    }
    ImGui::SetItemDefaultFocus();
    ImGui::SameLine();
    if (ImGui::Button("Exit Without Saving", ImVec2(150, 0))) {
      forceQuit = true;
      ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2(120, 0))) {
      ClearExitRequest();
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }
}

void DrawLoadConfirmation(Project &project) {
  if (triggerLoadConfirmation) {
    if (!project.isDirty) {
      if (Utils::LoadProject("project.d30", project)) {
        project.isDirty = false;
      }
      triggerLoadConfirmation = false;
    } else {
      ImGui::OpenPopup("ConfirmLoad");
    }
  }

  if (ImGui::BeginPopupModal("ConfirmLoad", NULL,
                             ImGuiWindowFlags_AlwaysAutoResize)) {
    ImGui::Text("You have unsaved changes! Do you want to save before "
                "loading a new project?\n\n");
    ImGui::Separator();

    if (ImGui::Button("Save and Load", ImVec2(120, 0))) {
      bool saveSuccess = false;
      if (project.csvFilePath.empty()) {
        saveSuccess = Utils::SaveProject(project);
      } else {
        saveSuccess = Utils::SaveProject(project, project.csvFilePath);
      }

      if (saveSuccess) {
        if (Utils::LoadProject("project.d30", project)) {
          project.isDirty = false;
        }
      }
      triggerLoadConfirmation = false;
      ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Discard and Load", ImVec2(150, 0))) {
      if (Utils::LoadProject("project.d30", project)) {
        project.isDirty = false;
      }
      triggerLoadConfirmation = false;
      ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2(120, 0))) {
      triggerLoadConfirmation = false;
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }
}

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

  if (ImGui::BeginMainMenuBar()) {

    // --- FILE MENU ---
    if (ImGui::BeginMenu("File")) {
      if (ImGui::MenuItem("Save")) {
        // If no file path is set, open a save dialog
        if (project.csvFilePath.empty()) {
          if (Utils::SaveProject(project)) {
            project.isDirty = false;
          }
        } else {
          // Otherwise, save to the existing path
          if (Utils::SaveProject(project, project.csvFilePath)) {
            project.isDirty = false;
          }
        }
      }
      if (ImGui::MenuItem("Save As...")) {
        if (Utils::SaveProject(project)) {
          project.isDirty = false;
        }
      }
      if (ImGui::MenuItem("Load Project")) {
        triggerLoadConfirmation = true;
      }

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
      if (ImGui::MenuItem("Exit")) {
        RequestExit();
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

  // --- 3. ICON LIBRARY POPUP ---
  if (triggerIconPopup) {
    ImGui::OpenPopup("IconLibraryPopup");
    triggerIconPopup = false;
    // Refresh purely to be safe, though constructor handles it
    // AssetManager::Get().RefreshLibrary();
  }

  // Set a nice big size for the library window
  ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_FirstUseEver);

  if (ImGui::BeginPopupModal("IconLibraryPopup", NULL, ImGuiWindowFlags_None)) {
    static int selectedCategory = 0;
    auto &categories = AssetManager::Get().GetCategories();

    if (categories.empty()) {
      ImGui::Text("No icons found in 'assets/icons'.");
      ImGui::Text("Please create folders and add PNG files.");
      if (ImGui::Button("Close"))
        ImGui::CloseCurrentPopup();
    } else {
      // --- LEFT COLUMN: Categories ---
      ImGui::BeginChild("Categories", ImVec2(150, 0), true);
      for (int i = 0; i < categories.size(); i++) {
        if (ImGui::Selectable(categories[i].name.c_str(),
                              selectedCategory == i)) {
          selectedCategory = i;
        }
      }
      ImGui::EndChild();

      ImGui::SameLine();

      // --- RIGHT COLUMN: Icons Grid ---
      ImGui::BeginChild("Icons", ImVec2(0, 0), true);

      // Load thumbnails for this category if needed
      AssetManager::Get().LoadCategoryTextures(selectedCategory);
      auto &currentCat = categories[selectedCategory];

      ImGuiStyle &style = ImGui::GetStyle();
      float windowVisibleX2 =
          ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;

      for (int i = 0; i < currentCat.icons.size(); i++) {
        Icon &icon = currentCat.icons[i];

        // Draw Image Button
        ImGui::PushID(i);
        // We use the thumbnail texture ID (void*)
        if (ImGui::ImageButton("icon_btn",
                               (ImTextureID)(intptr_t)icon.thumbnail.id,
                               ImVec2(48, 48))) {
          // ACTION: Add the icon as an ObjectType::Image
          // We load the FULL image from disk for the canvas, not the thumbnail
          project.objects.push_back({ObjectType::Image, 50, 50, 100, 100,
                                     currentCat.icons[i].path, "", "", 0,
                                     0xFFFFFFFF});
          project.isDirty = true;
          ImGui::CloseCurrentPopup();
        }
        ImGui::PopID();

        // Simple wrapping logic for the grid
        float lastButtonX2 = ImGui::GetItemRectMax().x;
        float nextButtonX2 =
            lastButtonX2 + style.ItemSpacing.x + 48; // Next button width
        if (i + 1 < currentCat.icons.size() && nextButtonX2 < windowVisibleX2)
          ImGui::SameLine();
      }
      ImGui::EndChild();
    }
    ImGui::EndPopup();
  }

  // 4. BORDER LIBRARY
  if (triggerBorderPopup) {
    ImGui::OpenPopup("BorderLibraryPopup");
    triggerBorderPopup = false;
  }
  ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_FirstUseEver);
  if (ImGui::BeginPopupModal("BorderLibraryPopup", NULL,
                             ImGuiWindowFlags_None)) {
    static int selectedBCat = 0;
    auto &categories = AssetManager::Get().GetBorders();
    if (categories.empty()) {
      ImGui::Text("No borders found in assets/borders");
      if (ImGui::Button("Close"))
        ImGui::CloseCurrentPopup();
    } else {
      ImGui::BeginChild("BCats", ImVec2(150, 0), true);
      for (int i = 0; i < categories.size(); i++) {
        if (ImGui::Selectable(categories[i].name.c_str(), selectedBCat == i))
          selectedBCat = i;
      }
      ImGui::EndChild();
      ImGui::SameLine();
      ImGui::BeginChild("BIcons", ImVec2(0, 0), true);
      AssetManager::Get().LoadBorderTextures(selectedBCat);
      auto &currentCat = categories[selectedBCat];
      float windowX2 =
          ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
      LabelSize sz = Utils::LabelSizes[project.selectedLabelIndex];
      for (int i = 0; i < currentCat.icons.size(); i++) {
        ImGui::PushID(i);
        if (ImGui::ImageButton(
                "border",
                (ImTextureID)(intptr_t)currentCat.icons[i].thumbnail.id,
                ImVec2(48, 48))) {
          project.objects.push_back(
              {ObjectType::Image, 0, 0, (float)sz.width, (float)sz.height,
               currentCat.icons[i].path, "", "", 0, 0xFFFFFFFF});
          project.isDirty = true;
          ImGui::CloseCurrentPopup();
        }
        ImGui::PopID();
        float nextX2 =
            ImGui::GetItemRectMax().x + ImGui::GetStyle().ItemSpacing.x + 48;
        if (i + 1 < currentCat.icons.size() && nextX2 < windowX2)
          ImGui::SameLine();
      }
      ImGui::EndChild();
    }

    // Close button at bottom? Or just click outside/top-right X (if enabled)
    // For Modal, we usually need a manual Close if we didn't pick anything
    if (ImGui::IsKeyPressed(ImGuiKey_Escape))
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
        project.isDirty = true;
      }
      if (is_selected)
        ImGui::SetItemDefaultFocus();
    }
    ImGui::EndCombo();
  }

  if (ImGui::Checkbox("Show Grid", &project.showGrid))
    project.isDirty = true;
  ImGui::SameLine();
  if (ImGui::Checkbox("Dark Mode", &project.darkTheme))
    project.isDirty = true;

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
    case ObjectType::Barcode:
      typePrefix = " [BRC] ";
      break;
    case ObjectType::Line:
      typePrefix = " [LN] ";
      break;
    case ObjectType::ShapeCircle:
      typePrefix = " [CIR] ";
      break;
    case ObjectType::ShapeRect:
      typePrefix = " [REC] ";
      break;
    case ObjectType::Border:
      typePrefix = " [BRD] ";
      break;
    default:
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

    if (ImGui::DragFloat("X", &obj.x))
      project.isDirty = true;
    if (ImGui::DragFloat("Y", &obj.y))
      project.isDirty = true;

    // Type-Specific Properties
    if (obj.type == ObjectType::Text || obj.type == ObjectType::Field) {
      // Import Font Button
      if (ImGui::Button("Import Font...")) {
        auto selection =
            pfd::open_file("Select Font", ".", {"Font Files", "*.ttf *.otf"})
                .result();
        if (!selection.empty())
          AssetManager::Get().ImportFont(selection[0]);
      }
      if (ImGui::SliderFloat("Font Size", &obj.fontSize, 10.0f, 100.0f))
        project.isDirty = true;
      if (ImGui::DragFloat("Box Width", &obj.width, 1.0f, 0.0f, 1000.0f, "%.1f"))
        project.isDirty = true;
      // Tooltip to explain
      if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Set > 0 to enable text wrapping");

      // Font Selector
      const auto &fontList = AssetManager::Get().GetFontList();
      std::string currentFont = obj.fontName.empty() ? "Default" : obj.fontName;
      if (ImGui::BeginCombo("Font", currentFont.c_str(),
                            ImGuiComboFlags_HeightLarge)) {
        if (ImGui::Selectable("Default", obj.fontName.empty())) {
          obj.fontName = "";
          project.isDirty = true;
        }

        bool hasUser = false;
        for (const auto &f : fontList) {
          if (f.type == LabelFontType::User) {
            if (!hasUser) {
              ImGui::Separator();
              ImGui::TextDisabled("--- User Fonts ---");
              hasUser = true;
            }
            if (ImGui::Selectable(f.name.c_str(), obj.fontName == f.name)) {
              obj.fontName = f.name;
              project.isDirty = true;
            }
          }
        }
        bool hasSystem = false;
        for (const auto &f : fontList) {
          if (f.type == LabelFontType::System) {
            if (!hasSystem) {
              ImGui::Separator();
              ImGui::TextDisabled("--- System Fonts ---");
              hasSystem = true;
            }
            if (ImGui::Selectable(f.name.c_str(), obj.fontName == f.name)) {
              obj.fontName = f.name;
              project.isDirty = true;
            }
          }
        }
        ImGui::EndCombo();
      }

    } else if (obj.type == ObjectType::Border ||
               obj.type == ObjectType::ShapeRect) {
      if (ImGui::SliderFloat("Thickness", &obj.fontSize, 1, 20))
        project.isDirty = true;
      if (ImGui::SliderFloat("Radius", &obj.cornerRadius, 0, 50))
        project.isDirty = true;
      if (ImGui::DragFloat("W", &obj.width))
        project.isDirty = true;
      if (ImGui::DragFloat("H", &obj.height))
        project.isDirty = true;

    } else if (obj.type == ObjectType::ShapeCircle ||
               obj.type == ObjectType::Line) {
      if (ImGui::SliderFloat("Line Thickness", &obj.fontSize, 1.0f, 20.0f))
        project.isDirty = true;
      if (ImGui::DragFloat("Width", &obj.width))
        project.isDirty = true;
      if (ImGui::DragFloat("Height", &obj.height))
        project.isDirty = true;
    } else if (obj.type == ObjectType::QRCode) {
      if (ImGui::DragFloat("Size", &obj.width, 1.0f, 10.0f, 500.0f)) {
        obj.height = obj.width; // Keep Square
        project.isDirty = true;
      }
    } else if (obj.type == ObjectType::Image) {
      if (ImGui::DragFloat("Width", &obj.width))
        project.isDirty = true;
      if (ImGui::DragFloat("Height", &obj.height))
        project.isDirty = true;

      ImGui::Spacing();
      if (ImGui::Button("Browse Image...")) {
        auto selection =
            pfd::open_file("Select Image", ".",
                           {"Image Files", "*.png *.jpg *.jpeg *.bmp"})
                .result();
        if (!selection.empty()) {
          obj.data = selection[0];
          project.isDirty = true;

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
    } else if (obj.type == ObjectType::Barcode) {
      if (ImGui::DragFloat("Width", &obj.width))
        project.isDirty = true;
      if (ImGui::DragFloat("Height", &obj.height))
        project.isDirty = true;
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
          project.isDirty = true;
        }

        for (const auto &header : project.csvHeaders) {
          bool isSelected = (obj.linkedColumn == header);
          if (ImGui::Selectable(header.c_str(), isSelected)) {
            obj.linkedColumn = header;
            project.isDirty = true;
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
    if (!ImGui::IsItemActive()) {
      strncpy(buffer, obj.data.c_str(), sizeof(buffer));
    }

    const char *label = (obj.type == ObjectType::QRCode)  ? "Data"
                        : (obj.type == ObjectType::Image) ? "File Path"
                                                          : "Text";
    if (ImGui::InputText(label, buffer, sizeof(buffer))) {
      obj.data = buffer;
      project.isDirty = true;
    }
  }

  ImGui::Spacing();
  ImGui::Separator();

  // --- 4. Add Buttons (Grid Layout) ---
  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Text("Tools");

  // We use a specific width (e.g. 110) to make buttons consistent
  ImVec2 btnSize(110, 0);

  // --- Row 1: Basics ---
  if (ImGui::Button("Add Text", btnSize)) {
    LabelObject obj;
    obj.type = ObjectType::Text;
    obj.x = 50;
    obj.y = 50;
    obj.data = "Text";
    obj.fontSize = 20;
    obj.colorHex = 0x000000FF;
    project.objects.push_back(obj);
    project.isDirty = true;
    selectedIndex = project.objects.size() - 1;
  }
  ImGui::SameLine();
  if (ImGui::Button("Add Field", btnSize)) {
    LabelObject obj;
    obj.type = ObjectType::Field;
    obj.x = 50;
    obj.y = 50;
    obj.data = "{Col}";
    obj.fontSize = 20;
    obj.colorHex = 0x000000FF;
    project.objects.push_back(obj);
    project.isDirty = true;
    selectedIndex = project.objects.size() - 1;
  }

  // --- Row 2: Media ---
  if (ImGui::Button("Add QR", btnSize)) {
    LabelObject obj;
    obj.type = ObjectType::QRCode;
    obj.x = 50;
    obj.y = 50;
    obj.width = 100;
    obj.height = 100;
    obj.data = "www.example.com";
    obj.colorHex = 0x000000FF;
    project.objects.push_back(obj);
    project.isDirty = true;
    selectedIndex = project.objects.size() - 1;
  }
  ImGui::SameLine();
  if (ImGui::Button("Add Barcode", btnSize)) {
    LabelObject obj;
    obj.type = ObjectType::Barcode;
    obj.x = 50;
    obj.y = 50;
    obj.width = 200;
    obj.height = 60;
    obj.data = "12345678";
    obj.colorHex = 0x000000FF;
    project.objects.push_back(obj);
    project.isDirty = true;
    selectedIndex = project.objects.size() - 1;
  }

  // --- Row 3: Graphics ---
  if (ImGui::Button("Add Image", btnSize)) {
    LabelObject obj;
    obj.type = ObjectType::Image;
    obj.x = 50;
    obj.y = 50;
    obj.width = 100;
    obj.height = 100;
    project.objects.push_back(obj);
    project.isDirty = true;
    selectedIndex = project.objects.size() - 1;
  }
  ImGui::SameLine();
  if (ImGui::Button("Add Icon", btnSize)) {
    triggerIconPopup = true; // Make sure this flag is accessible here!
  }

  // --- Row 4: Shapes ---
  if (ImGui::Button("Add Line", btnSize)) {
    LabelObject obj;
    obj.type = ObjectType::Line;
    obj.x = 50;
    obj.y = 50;
    obj.width = 100;
    obj.height = 0;
    obj.fontSize = 4;
    obj.colorHex = 0x000000FF;
    project.objects.push_back(obj);
    project.isDirty = true;
    selectedIndex = project.objects.size() - 1;
  }
  ImGui::SameLine();
  if (ImGui::Button("Add Rect", btnSize)) {
    LabelObject obj;
    obj.type = ObjectType::ShapeRect;
    obj.x = 50;
    obj.y = 50;
    obj.width = 100;
    obj.height = 100;
    obj.fontSize = 4;
    obj.colorHex = 0x000000FF;
    project.objects.push_back(obj);
    project.isDirty = true;
    selectedIndex = project.objects.size() - 1;
  }

  if (ImGui::Button("Add Circle", btnSize)) {
    LabelObject obj;
    obj.type = ObjectType::ShapeCircle;
    obj.x = 50;
    obj.y = 50;
    obj.width = 100;
    obj.height = 100;
    obj.fontSize = 4;
    obj.colorHex = 0x000000FF;
    project.objects.push_back(obj);
    project.isDirty = true;
    selectedIndex = project.objects.size() - 1;
  }

  // --- Row 5: Decor ---
  ImGui::SameLine();
  if (ImGui::Button("Add Border", btnSize)) {
    LabelSize sz = Utils::LabelSizes[project.selectedLabelIndex];
    LabelObject obj;
    obj.type = ObjectType::Border;
    obj.x = 4;
    obj.y = 4;
    obj.width = (float)sz.width - 8;
    obj.height = (float)sz.height - 8;
    obj.fontSize = 4;
    obj.colorHex = 0x000000FF;
    obj.cornerRadius = 10;
    project.objects.insert(project.objects.begin(), obj);
    project.isDirty = true;
    // Select the new object (which is now at index 0)
    selectedIndex = 0;
  }

  if (ImGui::Button("Deco Border", btnSize)) {
    triggerBorderPopup = true;
  }

  ImGui::End();
}
} // namespace UI