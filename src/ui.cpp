//  This file is part of Desktop-D30
//  Copyright (C) 2026 Chris Griffin (bearded-griffin)
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation version 3 of the License.
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <https://www.gnu.org/licenses/>.

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
#include "objects.h"
#include "printer.h"
#include "protocol.h"
#include "rlImGui.h"
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

namespace {
void DrawDeviceScanPopup(UIState &uiState);
void DrawBatchPrintPopup(Project &project, UIState &uiState);
void DrawIconLibraryPopup(Project &project, UIState &uiState);
void DrawBorderLibraryPopup(Project &project, UIState &uiState);
void DrawLibraryManager(UIState &uiState);
} // namespace

/*!***************************************************
 * @brief    Initializes the UI system
 * @details
 * @return   void
 * @note
 * @date     2026.02.01
 * @author   bearded.griffin
 ****************************************************/
void InitializeUI() {
  SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
  InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Desktop-D30");
  SetTargetFPS(60);

  rlImGuiSetup(true);
}

/*!***************************************************
 * @brief    Updates the window title
 * @details
 * @param    project const Project&
 * @return   void
 * @note
 * @date     2026.02.01
 * @author   bearded.griffin
 ****************************************************/
void UpdateWindowTitle(const Project &project) {
  std::string title = "Desktop-D30";
  if (!project.projectFilePath.empty()) {
    title += project.isDirty ? ": *" : ": ";
    size_t lastSlash = project.projectFilePath.find_last_of("/\\");
    title += (lastSlash != std::string::npos)
                 ? project.projectFilePath.substr(lastSlash + 1)
                 : project.projectFilePath;
  }
  SetWindowTitle(title.c_str());
}

/*!***************************************************
 * @brief    Requests application exit
 * @details
 * @return   void
 * @note
 * @date     2026.02.03
 * @author   bearded.griffin
 ****************************************************/
void RequestExit(UIState &uiState) { uiState.exitRequested = true; }

/*!***************************************************
 * @brief    Checks if application should close
 * @details
 * @return   bool
 * @note
 * @date     2026.02.03
 * @author   bearded.griffin
 ****************************************************/
bool ShouldClose(const UIState &uiState) { return uiState.forceQuit; }

/*!***************************************************
 * @brief    Clears the exit request flag
 * @details
 * @return   void
 * @note
 * @date     2026.02.03
 * @author   bearded.griffin
 ****************************************************/
void ClearExitRequest(UIState &uiState) { uiState.exitRequested = false; }

/*!***************************************************
 * @brief    Draws the exit confirmation dialog
 * @details
 * @param    project Project&
 * @return   void
 * @note
 * @date     2026.02.03
 * @author   bearded.griffin
 ****************************************************/
void DrawExitConfirmation(Project &project, UIState &uiState) {
  if (uiState.exitRequested) {
    if (!project.isDirty) {
      uiState.forceQuit = true;
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
        uiState.forceQuit = true;
      }
      ImGui::CloseCurrentPopup();
    }
    ImGui::SetItemDefaultFocus();
    ImGui::SameLine();
    if (ImGui::Button("Exit Without Saving", ImVec2(150, 0))) {
      uiState.forceQuit = true;
      ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2(120, 0))) {
      ClearExitRequest(uiState);
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }
}

/*!***************************************************
 * @brief    Draws the load confirmation dialog
 * @details
 * @param    project Project&
 * @return   void
 * @note
 * @date     2026.01.19
 * @author   bearded.griffin
 ****************************************************/
void DrawLoadConfirmation(Project &project, UIState &uiState) {
  if (uiState.triggerLoadConfirmation) {
    if (!project.isDirty) {
      if (Utils::LoadProject("project.d30", project)) {
        project.isDirty = false;
      }
      uiState.triggerLoadConfirmation = false;
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
      uiState.triggerLoadConfirmation = false;
      ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Discard and Load", ImVec2(150, 0))) {
      if (Utils::LoadProject("project.d30", project)) {
        project.isDirty = false;
      }
      uiState.triggerLoadConfirmation = false;
      ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2(120, 0))) {
      uiState.triggerLoadConfirmation = false;
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }
}

void DrawSplashScreen() {
  BeginDrawing();
  ClearBackground(RAYWHITE);

  int screenW = GetScreenWidth();
  int screenH = GetScreenHeight();

  // Draw Logo/Title
  const char *title = "Desktop-D30";
  int fontSize = 60;
  int textW = MeasureText(title, fontSize);
  DrawText(title, (screenW - textW) / 2, screenH / 2 - 100, fontSize, DARKGRAY);

  // Draw Progress Bar
  float progress = AssetManager::Get().GetLoadProgress();
  int barW = 600;
  int barH = 30;
  int barX = (screenW - barW) / 2;
  int barY = screenH / 2 + 50;

  DrawRectangleLines(barX, barY, barW, barH, LIGHTGRAY);
  DrawRectangle(barX + 2, barY + 2, (int)((barW - 4) * progress), barH - 4,
                SKYBLUE);

  // Fun Status Messages
  const char *statusMsg = "Loading Icons...";
  if (progress > 0.3f) statusMsg = "Wrangling the Angry Pixels...";
  if (progress > 0.6f) statusMsg = "Polishing the Pixels...";
  if (progress > 0.9f) statusMsg = "Finalizing Graphics...";
  
  int statusW = MeasureText(statusMsg, 24);
  DrawText(statusMsg, (screenW - statusW) / 2, barY - 35, 24, DARKGRAY);

  // Percentage Text
  std::string percentStr = std::to_string((int)(progress * 100)) + "%";
  int percentW = MeasureText(percentStr.c_str(), 20);
  DrawText(percentStr.c_str(), (screenW - percentW) / 2, barY + barH + 10, 20, GRAY);

  EndDrawing();
}

namespace {
void DrawDeviceScanPopup(UIState &uiState) {
  if (uiState.triggerScanPopup) {
    ImGui::OpenPopup("DeviceScanPopup");
    uiState.triggerScanPopup = false;
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
}

void DrawBatchPrintPopup(Project &project, UIState &uiState) {
  if (uiState.triggerBatchPopup) {
    ImGui::OpenPopup("BatchPrintPopup");
    uiState.triggerBatchPopup = false;
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
      Utils::BatchPrint(project, startRow, endRow);
      ImGui::CloseCurrentPopup();
    }

    ImGui::SameLine();
    if (ImGui::Button("Cancel"))
      ImGui::CloseCurrentPopup();

    ImGui::EndPopup();
  }
}

void DrawIconLibraryPopup(Project &project, UIState &uiState) {
  if (uiState.triggerIconPopup) {
    ImGui::OpenPopup("IconLibraryPopup");
    uiState.triggerIconPopup = false;
  }

  // Set a nice big size for the library window
  ImGui::SetNextWindowSize(ImVec2(800, 500), ImGuiCond_FirstUseEver);

  if (ImGui::BeginPopupModal("IconLibraryPopup", NULL, ImGuiWindowFlags_None)) {
    static int selectedCategory = 0;
    static char searchBuf[64] = "";
    auto &categories = AssetManager::Get().GetCategories();

    if (categories.empty()) {
      ImGui::Text("No icons found in 'assets/icons'.");
      if (ImGui::Button("Close"))
        ImGui::CloseCurrentPopup();
    } else {
      // --- TOP BAR: Search ---
      ImGui::InputText("Search", searchBuf, sizeof(searchBuf));
      ImGui::SameLine();
      if (ImGui::Button("Close"))
        ImGui::CloseCurrentPopup();
      ImGui::Separator();

      // --- LEFT COLUMN: Categories ---
      ImGui::BeginChild("Categories", ImVec2(150, 0), true);
      for (size_t i = 0; i < categories.size(); i++) {
        if (ImGui::Selectable(categories[i].name.c_str(),
                              selectedCategory == (int)i)) {
          selectedCategory = i;
        }
      }
      ImGui::EndChild();

      ImGui::SameLine();

      // --- RIGHT COLUMN: Icons Grid ---
      ImGui::BeginChild("Icons", ImVec2(0, 0), true);

      ImGuiStyle &style = ImGui::GetStyle();
      float windowVisibleX2 =
          ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;

      bool isSearching = (searchBuf[0] != '\0');
      std::string searchStr = searchBuf;
      if (isSearching) {
        std::transform(searchStr.begin(), searchStr.end(), searchStr.begin(),
                       ::tolower);
      }

      // Determine which categories to iterate over
      int startCat = isSearching ? 0 : selectedCategory;
      int endCat = isSearching ? (int)categories.size() : selectedCategory + 1;

      int iconDrawCount = 0;
      for (int catIdx = startCat; catIdx < endCat; catIdx++) {
        auto &currentCat = categories[catIdx];
        
        // We only load textures if we are actually going to display something from this cat
        // But for global search, we might load many. Given 128x128 icons, this is usually okay.
        bool catTexturesLoaded = false;

        for (size_t i = 0; i < currentCat.icons.size(); i++) {
          Icon &icon = currentCat.icons[i];

          // Search Filter
          if (isSearching) {
            std::string name =
                icon.customName.empty() ? icon.name : icon.customName;
            std::transform(name.begin(), name.end(), name.begin(), ::tolower);

            bool match = (name.find(searchStr) != std::string::npos);
            if (!match) {
              for (const auto &t : icon.tags) {
                std::string tag = t;
                std::transform(tag.begin(), tag.end(), tag.begin(), ::tolower);
                if (tag.find(searchStr) != std::string::npos) {
                  match = true;
                  break;
                }
              }
            }
            if (!match)
              continue;
          }

          // Ensure texture is loaded before drawing
          if (!catTexturesLoaded) {
              AssetManager::Get().LoadCategoryTextures(catIdx);
              catTexturesLoaded = true;
          }

          ImGui::PushID(icon.path.c_str());
          if (ImGui::ImageButton("icon_btn",
                                 (ImTextureID)(intptr_t)icon.thumbnail.id,
                                 ImVec2(48, 48))) {
            project.objects.push_back({ObjectType::Image, 50, 50, 100, 100,
                                       icon.path, "", "", 0, 0xFFFFFFFF});
            project.isDirty = true;
            ImGui::CloseCurrentPopup();
          }
          if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s\n(Category: %s)", 
                               icon.customName.empty() ? icon.name.c_str() : icon.customName.c_str(),
                               currentCat.name.c_str());
          }
          ImGui::PopID();

          iconDrawCount++;
          float lastButtonX2 = ImGui::GetItemRectMax().x;
          float nextButtonX2 = lastButtonX2 + style.ItemSpacing.x + 48;
          if (nextButtonX2 < windowVisibleX2)
            ImGui::SameLine();
        }
      }
      
      if (iconDrawCount == 0 && isSearching) {
          ImGui::Text("No icons match your search.");
      }

      ImGui::EndChild();
    }
    ImGui::EndPopup();
  }
}

void DrawBorderLibraryPopup(Project &project, UIState &uiState) {
  if (uiState.triggerBorderPopup) {
    ImGui::OpenPopup("BorderLibraryPopup");
    uiState.triggerBorderPopup = false;
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
      for (size_t i = 0; i < categories.size(); i++) {
        if (ImGui::Selectable(categories[i].name.c_str(),
                              selectedBCat == (int)i))
          selectedBCat = i;
      }
      ImGui::EndChild();
      ImGui::SameLine();
      ImGui::BeginChild("BIcons", ImVec2(0, 0), true);
      AssetManager::Get().LoadBorderTextures(selectedBCat);
      auto &currentCat = categories[selectedBCat];
      float windowX2 =
          ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
      LabelSize sz = LabelSizes[project.selectedLabelIndex];
      for (size_t i = 0; i < currentCat.icons.size(); i++) {
        ImGui::PushID(i);
        if (ImGui::ImageButton(
                "border",
                (ImTextureID)(intptr_t)currentCat.icons[i].thumbnail.id,
                ImVec2(48, 48))) {
          project.objects.push_back({ObjectType::Image, 0, 0, (float)sz.width,
                                     (float)sz.height, currentCat.icons[i].path,
                                     "", "", 0, 0xFFFFFFFF});
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

    if (ImGui::IsKeyPressed(ImGuiKey_Escape))
      ImGui::CloseCurrentPopup();

    ImGui::EndPopup();
  }
}

void DrawLibraryManager(UIState &uiState) {
  if (uiState.triggerLibraryManager) {
    ImGui::OpenPopup("Library Manager");
    uiState.triggerLibraryManager = false;
  }

  ImGui::SetNextWindowSize(ImVec2(1000, 700), ImGuiCond_FirstUseEver);
  if (ImGui::BeginPopupModal("Library Manager", NULL, ImGuiWindowFlags_None)) {
    static char searchBuf[128] = "";
    static int selectedCatIdx = 0;
    static Icon *selectedIcon = nullptr;
    static char nameBuf[128] = "";
    static char tagBuf[128] = "";

    auto &categories = AssetManager::Get().GetCategories();

    // --- TOP BAR: Search ---
    ImGui::InputText("Search Icons", searchBuf, sizeof(searchBuf));
    ImGui::SameLine();
    if (ImGui::Button("Import Icons...")) {
      auto selection = pfd::open_file("Import Icons", ".",
                                      {"Images", "*.png *.jpg *.jpeg *.bmp"},
                                      pfd::opt::multiselect)
                           .result();
      if (!selection.empty()) {
        int count = AssetManager::Get().ImportUserIcons(selection);
        std::cout << "[UI] Imported " << count << " icons." << std::endl;
      }
    }
    ImGui::SameLine();
    if (ImGui::Button("Close"))
      ImGui::CloseCurrentPopup();
    ImGui::Separator();

    // --- LEFT COLUMN: Categories ---
    ImGui::BeginChild("LibraryCategories", ImVec2(200, 0), true);
    for (size_t i = 0; i < categories.size(); i++) {
      if (ImGui::Selectable(categories[i].name.c_str(),
                            selectedCatIdx == (int)i)) {
        selectedCatIdx = i;
        selectedIcon = nullptr;
      }
    }
    ImGui::EndChild();

    ImGui::SameLine();

    // --- MIDDLE COLUMN: Icon Grid ---
    ImGui::BeginChild("LibraryGrid", ImVec2(500, 0), true);
    
    bool isSearching = (searchBuf[0] != '\0');
    std::string searchStr = searchBuf;
    if (isSearching) {
      std::transform(searchStr.begin(), searchStr.end(), searchStr.begin(),
                     ::tolower);
    }

    int startCat = isSearching ? 0 : selectedCatIdx;
    int endCat = isSearching ? (int)categories.size() : selectedCatIdx + 1;

    for (int catIdx = startCat; catIdx < endCat; catIdx++) {
      auto &cat = categories[catIdx];
      bool catTexturesLoaded = false;

      float windowVisibleX2 =
          ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;

      for (size_t i = 0; i < cat.icons.size(); i++) {
        Icon &icon = cat.icons[i];

        // Filter by search
        if (isSearching) {
          std::string iconName = icon.name;
          std::transform(iconName.begin(), iconName.end(), iconName.begin(),
                         ::tolower);
          std::string custName = icon.customName;
          std::transform(custName.begin(), custName.end(), custName.begin(), ::tolower);

          bool match = (iconName.find(searchStr) != std::string::npos ||
                        custName.find(searchStr) != std::string::npos);
          if (!match) {
            for (const auto &t : icon.tags) {
              std::string tag = t;
              std::transform(tag.begin(), tag.end(), tag.begin(), ::tolower);
              if (tag.find(searchStr) != std::string::npos) {
                match = true;
                break;
              }
            }
          }
          if (!match)
            continue;
        }

        if (!catTexturesLoaded) {
            AssetManager::Get().LoadCategoryTextures(catIdx);
            catTexturesLoaded = true;
        }

        ImGui::PushID(icon.path.c_str());
        bool isSelected = (selectedIcon == &icon);
        if (isSelected)
          ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.4f, 0.6f, 1.0f));

        if (ImGui::ImageButton("lib_icon",
                               (ImTextureID)(intptr_t)icon.thumbnail.id,
                               ImVec2(64, 64))) {
          selectedIcon = &icon;
          strncpy(nameBuf, icon.customName.c_str(), sizeof(nameBuf));
        }

        if (isSelected)
          ImGui::PopStyleColor();
        ImGui::PopID();

        float lastX2 = ImGui::GetItemRectMax().x;
        float nextX2 = lastX2 + ImGui::GetStyle().ItemSpacing.x + 64;
        if (nextX2 < windowVisibleX2)
          ImGui::SameLine();
      }
    }
    ImGui::EndChild();

    ImGui::SameLine();

    // --- RIGHT COLUMN: Inspector ---
    ImGui::BeginChild("LibraryInspector", ImVec2(0, 0), true);
    if (selectedIcon) {
      ImGui::Text("Icon Details");
      ImGui::Separator();
      ImGui::Image((ImTextureID)(intptr_t)selectedIcon->thumbnail.id,
                   ImVec2(128, 128));
      ImGui::Text("File: %s",
                  fs::path(selectedIcon->path).filename().string().c_str());

      ImGui::Spacing();
      if (ImGui::InputText("Display Name", nameBuf, sizeof(nameBuf))) {
        selectedIcon->customName = nameBuf;
        AssetManager::Get().SaveMetadata();
      }

      ImGui::Spacing();
      ImGui::Text("Tags:");
      for (size_t t = 0; t < selectedIcon->tags.size(); t++) {
        ImGui::Text(" [#] %s", selectedIcon->tags[t].c_str());
        ImGui::SameLine();
        if (ImGui::SmallButton(("X##" + std::to_string(t)).c_str())) {
          selectedIcon->tags.erase(selectedIcon->tags.begin() + t);
          AssetManager::Get().SaveMetadata();
        }
      }

      ImGui::InputText("Add Tag", tagBuf, sizeof(tagBuf));
      ImGui::SameLine();
      if (ImGui::Button("+")) {
        if (tagBuf[0] != '\0') {
          selectedIcon->tags.push_back(tagBuf);
          AssetManager::Get().SaveMetadata();
          tagBuf[0] = '\0';
        }
      }

      ImGui::Separator();
      ImGui::Text("Move to Category:");
      static int moveCatIdx = 0;
      if (ImGui::BeginCombo("##MoveCombo", categories[moveCatIdx].name.c_str())) {
        for (int n = 0; n < (int)categories.size(); n++) {
          if (ImGui::Selectable(categories[n].name.c_str(), moveCatIdx == n))
            moveCatIdx = n;
        }
        ImGui::EndCombo();
      }
      if (ImGui::Button("Perform Move")) {
        AssetManager::Get().MoveIcon(*selectedIcon,
                                     categories[moveCatIdx].name);
        selectedIcon = nullptr; // Reset selection as pointers might change
      }
    } else {
      ImGui::TextDisabled("Select an icon to edit metadata.");
    }
    ImGui::EndChild();

    ImGui::EndPopup();
  }
}
} // namespace

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
void DrawMainMenu(Project &project, UIState &uiState) {

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
        uiState.triggerLoadConfirmation = true;
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
          uiState.triggerBatchPopup = true;
        }
      }

      ImGui::Separator();
      if (ImGui::MenuItem("Exit")) {
        RequestExit(uiState);
      }
      ImGui::EndMenu();
    }

    // --- ASSETS MENU ---
    if (ImGui::BeginMenu("Assets")) {
      if (ImGui::MenuItem("Manage Icon Library")) {
        uiState.triggerLibraryManager = true;
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
        uiState.triggerScanPopup = true;
      }
      ImGui::EndMenu();
    }

    ImGui::EndMainMenuBar();
  }

  // Draw Popups
  DrawDeviceScanPopup(uiState);
  DrawBatchPrintPopup(project, uiState);
  DrawIconLibraryPopup(project, uiState);
  DrawBorderLibraryPopup(project, uiState);
  DrawLibraryManager(uiState);
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
namespace {
// Add a forward declaration for DrawPropertiesPanel since it's used in this
// file.
void DrawPropertiesPanel(Project &project, int &selectedIndex,
                         UIState &uiState);

void DrawProjectSettings(Project &project) {
  ImGui::Text("Project Settings");
  ImGui::Separator();

  const char *currentItem = LabelSizes[project.selectedLabelIndex].name.c_str();
  if (ImGui::BeginCombo("Canvas Size", currentItem)) {
    for (size_t i = 0; i < LabelSizes.size(); i++) {
      bool isSelected = (project.selectedLabelIndex == (int)i);
      if (ImGui::Selectable(LabelSizes[i].name.c_str(), isSelected)) {
        project.selectedLabelIndex = i;
        project.isDirty = true;
      }
      if (isSelected)
        ImGui::SetItemDefaultFocus();
    }
    ImGui::EndCombo();
  }

  if (ImGui::Checkbox("Show Grid", &Utils::appSettings.showGrid)) {
    project.isDirty = true;
    Utils::SaveSettings(Utils::appSettings);
  }
  ImGui::SameLine();
  if (ImGui::Checkbox("Dark Mode", &Utils::appSettings.darkTheme)) {
    project.isDirty = true;
    Utils::SaveSettings(Utils::appSettings);
  }
}

void DrawDataSource(Project &project) {
  if (!project.csvRows.empty()) {
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::TextColored(ImVec4(0, 1, 1, 1), "Data Source");

    std::string filename = project.csvFilePath;
    size_t lastSlash = filename.find_last_of("/\\");
    if (lastSlash != std::string::npos)
      filename = filename.substr(lastSlash + 1);
    ImGui::Text("File: %s", filename.c_str());

    ImGui::Spacing();
    if (ImGui::Button("<<")) {
      project.currentCSVRow--;
      Utils::ApplyCSVDataToObjects(project);
    }

    ImGui::SameLine();
    ImGui::Text(" Row %d of %zu ", project.currentCSVRow + 1,
                project.csvRows.size());

    ImGui::SameLine();
    if (ImGui::Button(">>")) {
      project.currentCSVRow++;
      Utils::ApplyCSVDataToObjects(project);
    }

    int tempRow = project.currentCSVRow + 1;
    if (ImGui::SliderInt("##RowSlider", &tempRow, 1,
                         (int)project.csvRows.size())) {
      project.currentCSVRow = tempRow - 1;
      Utils::ApplyCSVDataToObjects(project);
    }
  }
}

void DrawObjectTree(Project &project, int &selectedIndex) {
  ImGui::Spacing();
  ImGui::Text("Objects Tree");
  ImGui::Separator();

  for (size_t i = 0; i < project.objects.size(); i++) {
    LabelObject &obj = project.objects[i];
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

    if (ImGui::Selectable(id.c_str(), selectedIndex == (int)i)) {
      selectedIndex = i;
    }
  }
}

void DrawPropertiesPanel(Project &project, int &selectedIndex,
                         UIState &uiState) {
  ImGui::Spacing();
  ImGui::Separator();

  if (selectedIndex >= 0 && selectedIndex < (int)project.objects.size()) {
    LabelObject &obj = project.objects[selectedIndex];
    ImGui::Text("Properties");

    if (ImGui::DragFloat("X", &obj.x))
      project.isDirty = true;
    if (ImGui::DragFloat("Y", &obj.y))
      project.isDirty = true;

    // Type-Specific Properties
    if (obj.type == ObjectType::Text || obj.type == ObjectType::Field) {
      if (ImGui::Button("Import Font...")) {
        auto selection =
            pfd::open_file("Select Font", ".", {"Font Files", "*.ttf *.otf"})
                .result();
        if (!selection.empty())
          AssetManager::Get().ImportFont(selection[0]);
      }
      if (ImGui::SliderFloat("Font Size", &obj.fontSize, 10.0f, 100.0f))
        project.isDirty = true;
      if (ImGui::DragFloat("Box Width", &obj.width, 1.0f, 0.0f, 1000.0f,
                           "%.1f"))
        project.isDirty = true;
      if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Set > 0 to enable text wrapping");

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
          if (obj.texture.id != 0)
            UnloadTexture(obj.texture);
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
}
} // namespace

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
void DrawSidebar(Project &project, int &selectedIndex, UIState &uiState) {
  ImGui::Begin("Inspector", nullptr,
               ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);
  ImGui::SetWindowPos({0, 20}, ImGuiCond_FirstUseEver);
  ImGui::SetWindowSize({300, 600}, ImGuiCond_FirstUseEver);

  DrawProjectSettings(project);
  DrawDataSource(project);
  DrawObjectTree(project, selectedIndex);
  DrawPropertiesPanel(project, selectedIndex, uiState);

  // --- 4. Add Buttons (Grid Layout) ---
  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Text("Tools");

  // We use a specific width (e.g. 110) to make buttons consistent
  ImVec2 btnSize(110, 0);

  // --- Row 1: Basics ---
  if (ImGui::Button("Add Text", btnSize)) {
    LabelObject obj = OBJECTS::CreateTextObject(50, 50, "Text", 20);
    project.objects.push_back(obj);
    project.isDirty = true;
    selectedIndex = project.objects.size() - 1;
  }
  ImGui::SameLine();
  if (ImGui::Button("Add Field", btnSize)) {
    LabelObject obj = OBJECTS::CreateFieldObject(50, 50, "{Col}", 20);
    project.objects.push_back(obj);
    project.isDirty = true;
    selectedIndex = project.objects.size() - 1;
  }

  // --- Row 2: Media ---
  if (ImGui::Button("Add QR", btnSize)) {
    LabelObject obj =
        OBJECTS::CreateQRCodeObject(50, 50, 100, "www.example.com");
    project.objects.push_back(obj);
    project.isDirty = true;
    selectedIndex = project.objects.size() - 1;
  }
  ImGui::SameLine();
  if (ImGui::Button("Add Barcode", btnSize)) {
    LabelObject obj = OBJECTS::CreateBarcodeObject(50, 50, 200, 60, "12345678");
    project.objects.push_back(obj);
    project.isDirty = true;
    selectedIndex = project.objects.size() - 1;
  }

  // --- Row 3: Graphics ---
  if (ImGui::Button("Add Image", btnSize)) {
    auto selection =
        pfd::open_file("Select Image", ".",
                       {"Image Files", "*.png *.jpg *.jpeg *.bmp"})
            .result();
    if (!selection.empty()) {
      LabelObject obj =
          OBJECTS::CreateImageObject(50, 50, 100, 100, selection[0]);

      // Immediately load and constrain image for display
      Image img = LoadImage(obj.data.c_str());
      if (img.data != NULL) {
        // --- CONSTRAIN IMAGE SIZE ---
        // If image is huge, downscale it before creating a texture
        // This prevents crashes and saves GPU memory
        const int MAX_IMPORT_SIZE = 512;
        if (img.width > MAX_IMPORT_SIZE || img.height > MAX_IMPORT_SIZE) {
          float aspect = (float)img.width / (float)img.height;
          int newW, newH;
          if (img.width > img.height) {
            newW = MAX_IMPORT_SIZE;
            newH = (int)(MAX_IMPORT_SIZE / aspect);
          } else {
            newH = MAX_IMPORT_SIZE;
            newW = (int)(MAX_IMPORT_SIZE * aspect);
          }
          ImageResize(&img, newW, newH);
        }

        // Set initial logical dimensions to match constrained image
        obj.width = (float)img.width;
        obj.height = (float)img.height;

        project.objects.push_back(obj);
        project.isDirty = true;
        selectedIndex = project.objects.size() - 1;

        project.objects[selectedIndex].texture = LoadTextureFromImage(img);
        UnloadImage(img);
      }
    }
  }
  ImGui::SameLine();
  if (ImGui::Button("Add Icon", btnSize)) {
    uiState.triggerIconPopup = true;
  }

  // --- Row 4: Shapes ---
  if (ImGui::Button("Add Line", btnSize)) {
    LabelObject obj = OBJECTS::CreateLineObject(50, 50, 100, 0, 4);
    project.objects.push_back(obj);
    project.isDirty = true;
    selectedIndex = project.objects.size() - 1;
  }
  ImGui::SameLine();
  if (ImGui::Button("Add Rect", btnSize)) {
    LabelObject obj = OBJECTS::CreateRectangleObject(50, 50, 100, 100, 4);
    project.objects.push_back(obj);
    project.isDirty = true;
    selectedIndex = project.objects.size() - 1;
  }

  if (ImGui::Button("Add Circle", btnSize)) {
    LabelObject obj = OBJECTS::CreateCircleObject(50, 50, 25);
    obj.type = ObjectType::ShapeCircle;
    project.objects.push_back(obj);
    project.isDirty = true;
    selectedIndex = project.objects.size() - 1;
  }

  // --- Row 5: Decor ---
  ImGui::SameLine();
  if (ImGui::Button("Add Border", btnSize)) {
    LabelSize sz = LabelSizes[project.selectedLabelIndex];
    LabelObject obj = OBJECTS::CreateBorderObject(4, 4, sz);
    project.objects.insert(project.objects.begin(), obj);
    project.isDirty = true;
    selectedIndex = 0;
  }

  if (ImGui::Button("Deco Border", btnSize)) {
    uiState.triggerBorderPopup = true;
  }

  ImGui::End();
}

void Draw(Project &project, int &selectedIndex, UIState &uiState) {
  rlImGuiBegin();
  DrawMainMenu(project, uiState);
  DrawSidebar(project, selectedIndex, uiState);
  DrawExitConfirmation(project, uiState);
  DrawLoadConfirmation(project, uiState);
  rlImGuiEnd();

  EndDrawing();
}

/*!***************************************************
 * @brief    Cleans up the UI system
 * @details
 * @param    currentProject Project&
 * @return   void
 * @note
 * @date     2026.02.03
 * @author   bearded.griffin
 ****************************************************/
void CleanupApplication(Project &currentProject) {
  // Cleanup
  for (auto &obj : currentProject.objects) {
    if (obj.texture.id != 0) {
      UnloadTexture(obj.texture);
    }
  }

  rlImGuiShutdown();
  CloseWindow();
}
} // namespace UI