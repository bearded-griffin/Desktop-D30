//  This file is part of Desktop-D30
//  Copyright (C) 2026 bearded-griffin
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
namespace ExitDialog {
constexpr const char *TITLE = "ConfirmExit";
constexpr const char *MESSAGE = "You have unsaved changes!\n\n";
constexpr const char *SAVE_BUTTON = "Save and Exit";
constexpr const char *DISCARD_BUTTON = "Exit Without Saving";
constexpr const char *CANCEL_BUTTON = "Cancel";
constexpr ImVec2 BUTTON_SIZE(120, 0);
constexpr ImVec2 WIDE_BUTTON_SIZE(150, 0);
} // namespace ExitDialog

namespace Cleanup {
constexpr const char *LOG_PREFIX = "[Cleanup]";
constexpr bool LOG_OPERATIONS = true;
} // namespace Cleanup

namespace Colors {
constexpr Color Background = RAYWHITE;
constexpr Color Title = DARKGRAY;
constexpr Color ProgressBar = SKYBLUE;
constexpr Color ProgressBarBorder = LIGHTGRAY;
constexpr Color StatusText = DARKGRAY;
constexpr Color PercentText = GRAY;
} // namespace Colors

namespace Layout {
constexpr float TitleFontSize = 60.0f;
constexpr float StatusFontSize = 24.0f;
constexpr float PercentFontSize = 20.0f;
constexpr float TextSpacing = 2.0f;
constexpr int ProgressBarWidth = 600;
constexpr int ProgressBarHeight = 30;
constexpr int TitleOffsetY = 100;
constexpr int ProgressBarOffsetY = 50;
constexpr int StatusOffsetY = 35;
constexpr int PercentOffsetY = 10;
} // namespace Layout

namespace AboutDialog {
constexpr const char *TITLE = "About Desktop-D30";
constexpr const char *APP_NAME = "Desktop-D30 - Label Design Software";
constexpr const char *VERSION = "Version 1.0.0";
constexpr const char *AUTHOR = "Created by bearded-griffin";
constexpr const char *LICENSE = "Licensed under GNU GPL v3.0";
constexpr const char *SUPPORT_TEXT =
    "If you enjoy this app, please consider supporting development:";
constexpr const char *SUPPORT_BUTTON = "Support on Ko-fi";
constexpr const char *CLOSE_BUTTON = "Close";
constexpr const char *SUPPORT_URL = "https://ko-fi.com/beardedgriffin";
constexpr ImVec2 BUTTON_SIZE(120, 0);
} // namespace AboutDialog

namespace {
void DrawDeviceScanPopup(UIState &uiState);
void DrawBatchPrintPopup(Project &project, UIState &uiState);
void DrawSequencePrintPopup(Project &project, UIState &uiState);
void DrawIconLibraryPopup(Project &project, UIState &uiState,
                          InteractionState &state);
void DrawBorderLibraryPopup(Project &project, UIState &uiState,
                            InteractionState &state);
void DrawLibraryManager(UIState &uiState);
void DrawBorderManager(UIState &uiState);
void PerformCopyOperation(Project &project, InteractionState &state);
void PerformPasteOperation(Project &project, InteractionState &state);
void PerformCutOperation(Project &project, InteractionState &state);
void PerformDeleteOperation(Project &project, InteractionState &state);

/*!***************************************************
 * @brief    Draws the Bluetooth device scan popup
 * @param    uiState UIState&
 * @date     2026.02.19
 ****************************************************/
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

/*!***************************************************
 * @brief    Draws the Batch Print (CSV) popup
 * @param    project Project&
 * @param    uiState UIState&
 * @date     2026.02.19
 ****************************************************/
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

/*!***************************************************
 * @brief    Draws the Sequence Print (Auto-Inc) popup
 * @param    project Project&
 * @param    uiState UIState&
 * @date     2026.02.19
 ****************************************************/
void DrawSequencePrintPopup(Project &project, UIState &uiState) {
  if (uiState.triggerSequencePopup) {
    ImGui::OpenPopup("SequencePrintPopup");
    uiState.triggerSequencePopup = false;
  }

  if (ImGui::BeginPopupModal("SequencePrintPopup", NULL,
                             ImGuiWindowFlags_AlwaysAutoResize)) {
    static int labelCount = 1;

    ImGui::Text("Print Auto-Increment Sequence");
    ImGui::Separator();

    ImGui::InputInt("Number of Labels", &labelCount);
    if (labelCount < 1)
      labelCount = 1;

    ImGui::Separator();

    if (ImGui::Button("START PRINTING")) {
      Utils::SequencePrint(project, labelCount);
      ImGui::CloseCurrentPopup();
    }

    ImGui::SameLine();
    if (ImGui::Button("Cancel"))
      ImGui::CloseCurrentPopup();

    ImGui::EndPopup();
  }
}

/*!***************************************************
 * @brief    Draws the Icon Library picker popup
 * @param    project Project&
 * @param    uiState UIState&
 * @param    state InteractionState&
 * @date     2026.02.19
 ****************************************************/
void DrawIconLibraryPopup(Project &project, UIState &uiState,
                          InteractionState &state) {
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
      if (selectedCategory >= (int)categories.size())
        selectedCategory = 0;

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

        // We only load textures if we are actually going to display something
        // from this cat But for global search, we might load many. Given
        // 128x128 icons, this is usually okay.
        bool catTexturesLoaded = false;

        for (size_t i = 0; i < currentCat.icons.size(); i++) {
          Icon &icon = currentCat.icons[i];

          // Search Filter
          if (isSearching) {
            std::string name = icon.name;
            std::string cname = icon.customName;
            std::transform(name.begin(), name.end(), name.begin(), ::tolower);
            std::transform(cname.begin(), cname.end(), cname.begin(),
                           ::tolower);

            bool match = (name.find(searchStr) != std::string::npos ||
                          cname.find(searchStr) != std::string::npos);
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
            state.PushHistory(project);
            LabelObject obj = {ObjectType::Image, 0,  0,  60, 60,
                               icon.path,         "", "", 0,  0xFFFFFFFF};
            obj.data = icon.path;
            project.objects.push_back(obj);
            project.isDirty = true;

            state.selectedIndices.clear();
            state.selectedIndices.push_back((int)project.objects.size() - 1);

            ImGui::CloseCurrentPopup();
          }
          if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s\n(Category: %s)",
                              icon.customName.empty() ? icon.name.c_str()
                                                      : icon.customName.c_str(),
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

/*!***************************************************
 * @brief    Draws the Border Library picker popup
 * @param    project Project&
 * @param    uiState UIState&
 * @param    state InteractionState&
 * @date     2026.02.19
 ****************************************************/
void DrawBorderLibraryPopup(Project &project, UIState &uiState,
                            InteractionState &state) {
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
      if (selectedBCat >= (int)categories.size())
        selectedBCat = 0;

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
        Icon &icon = currentCat.icons[i];
        ImGui::PushID(i);
        if (ImGui::ImageButton("border",
                               (ImTextureID)(intptr_t)icon.thumbnail.id,
                               ImVec2(48, 48))) {
          state.PushHistory(project);
          LabelObject obj = {ObjectType::Image,
                             0,
                             0,
                             (float)sz.width,
                             (float)sz.height,
                             icon.path,
                             "",
                             "",
                             0,
                             0xFFFFFFFF};
          obj.data = icon.path;
          project.objects.insert(project.objects.begin(), obj);
          project.isDirty = true;

          state.selectedIndices.clear();
          state.selectedIndices.push_back(0); // It was inserted at the front

          ImGui::CloseCurrentPopup();
        }
        if (ImGui::IsItemHovered()) {
          ImGui::SetTooltip("%s", icon.customName.empty()
                                      ? icon.name.c_str()
                                      : icon.customName.c_str());
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

/*!***************************************************
 * @brief    Draws the Icon Library Management window
 * @param    uiState UIState&
 * @date     2026.02.19
 ****************************************************/
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
    if (categories.empty()) {
      ImGui::Text("No icons found.");
    } else {
      if (selectedCatIdx >= (int)categories.size())
        selectedCatIdx = 0;

      for (size_t i = 0; i < categories.size(); i++) {
        if (ImGui::Selectable(categories[i].name.c_str(),
                              selectedCatIdx == (int)i)) {
          selectedCatIdx = i;
          selectedIcon = nullptr;
        }
      }
    }
    ImGui::EndChild();

    ImGui::SameLine();

    // --- MIDDLE COLUMN: Icon Grid ---
    ImGui::BeginChild("LibraryGrid", ImVec2(500, 0), true);

    if (categories.empty()) {
      ImGui::Text("Please import icons or check assets folder.");
    } else {
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
            std::transform(custName.begin(), custName.end(), custName.begin(),
                           ::tolower);

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
            ImGui::PushStyleColor(ImGuiCol_Button,
                                  ImVec4(0.2f, 0.4f, 0.6f, 1.0f));

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
      if (moveCatIdx >= (int)categories.size())
        moveCatIdx = 0;

      if (!categories.empty()) {
        if (ImGui::BeginCombo("##MoveCombo",
                              categories[moveCatIdx].name.c_str())) {
          for (int n = 0; n < (int)categories.size(); n++) {
            if (ImGui::Selectable(categories[n].name.c_str(), moveCatIdx == n))
              moveCatIdx = n;
          }
          ImGui::EndCombo();
        }
        if (ImGui::Button("Perform Move")) {
          AssetManager::Get().MoveAsset(*selectedIcon,
                                        categories[moveCatIdx].name);
          selectedIcon = nullptr; // Reset selection as pointers might change
        }
      } else {
        ImGui::TextDisabled("No categories available.");
      }
    } else {
      ImGui::TextDisabled("Select an icon to edit metadata.");
    }
    ImGui::EndChild();

    ImGui::EndPopup();
  }
}

/*!***************************************************
 * @brief    Draws the Border Management window
 * @param    uiState UIState&
 * @date     2026.02.19
 ****************************************************/
void DrawBorderManager(UIState &uiState) {
  if (uiState.triggerBorderManager) {
    ImGui::OpenPopup("Border Manager");
    uiState.triggerBorderManager = false;
  }

  ImGui::SetNextWindowSize(ImVec2(1000, 700), ImGuiCond_FirstUseEver);
  if (ImGui::BeginPopupModal("Border Manager", NULL, ImGuiWindowFlags_None)) {
    static char searchBuf[128] = "";
    static int selectedCatIdx = 0;
    static Icon *selectedIcon = nullptr;
    static char nameBuf[128] = "";
    static char tagBuf[128] = "";

    auto &categories = AssetManager::Get().GetBorders();

    ImGui::InputText("Search Borders", searchBuf, sizeof(searchBuf));
    ImGui::SameLine();
    if (ImGui::Button("Import Borders...")) {
      auto selection = pfd::open_file("Import Borders", ".",
                                      {"Images", "*.png *.jpg *.jpeg *.bmp"},
                                      pfd::opt::multiselect)
                           .result();
      if (!selection.empty()) {
        AssetManager::Get().ImportUserBorders(selection);
      }
    }
    ImGui::SameLine();
    if (ImGui::Button("Close"))
      ImGui::CloseCurrentPopup();
    ImGui::Separator();

    ImGui::BeginChild("BorderCategories", ImVec2(200, 0), true);
    if (categories.empty()) {
      ImGui::Text("No borders found.");
    } else {
      if (selectedCatIdx >= (int)categories.size())
        selectedCatIdx = 0;

      for (size_t i = 0; i < categories.size(); i++) {
        if (ImGui::Selectable(categories[i].name.c_str(),
                              selectedCatIdx == (int)i)) {
          selectedCatIdx = i;
          selectedIcon = nullptr;
        }
      }
    }
    ImGui::EndChild();

    ImGui::SameLine();

    // --- MIDDLE COLUMN: Border Grid ---
    ImGui::BeginChild("BorderGrid", ImVec2(500, 0), true);

    if (categories.empty()) {
      ImGui::Text("Please import borders or check assets folder.");
    } else {
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

          if (isSearching) {
            std::string iconName = icon.name;
            std::transform(iconName.begin(), iconName.end(), iconName.begin(),
                           ::tolower);
            std::string custName = icon.customName;
            std::transform(custName.begin(), custName.end(), custName.begin(),
                           ::tolower);

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
            AssetManager::Get().LoadBorderTextures(catIdx);
            catTexturesLoaded = true;
          }

          ImGui::PushID(icon.path.c_str());
          bool isSelected = (selectedIcon == &icon);
          if (isSelected)
            ImGui::PushStyleColor(ImGuiCol_Button,
                                  ImVec4(0.2f, 0.4f, 0.6f, 1.0f));

          if (ImGui::ImageButton("lib_border",
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
    }
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("BorderInspector", ImVec2(0, 0), true);
    if (selectedIcon) {
      ImGui::Text("Border Details");
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
      static int moveBorderCatIdx = 0;
      if (moveBorderCatIdx >= (int)categories.size())
        moveBorderCatIdx = 0;

      if (!categories.empty()) {
        if (ImGui::BeginCombo("##MoveBorderCombo",
                              categories[moveBorderCatIdx].name.c_str())) {
          for (int n = 0; n < (int)categories.size(); n++) {
            if (ImGui::Selectable(categories[n].name.c_str(),
                                  moveBorderCatIdx == n))
              moveBorderCatIdx = n;
          }
          ImGui::EndCombo();
        }
        if (ImGui::Button("Perform Move")) {
          AssetManager::Get().MoveAsset(*selectedIcon,
                                        categories[moveBorderCatIdx].name);
          selectedIcon = nullptr;
        }
      } else {
        ImGui::TextDisabled("No categories available.");
      }
    } else {
      ImGui::TextDisabled("Select a border to edit metadata.");
    }
    ImGui::EndChild();

    ImGui::EndPopup();
  }
}

/*!***************************************************
 * @brief    Draws the properties panel
 * @details  It draws the side "Inspector" bar on the left side.
 * @param    project Project&
 * @param    state InteractionState&
 * @param    uiState UIState&
 * @return   void
 * @note     Add a forward declaration for DrawpropertiesPanel since it's used
 * in this file but defined in another file.
 * @date     2026.01.19
 ****************************************************/
void DrawPropertiesPanel(Project &project, InteractionState &state,
                         UIState &uiState);

/*!***************************************************
 * @brief    Draws the Project Settings panel
 * @param    project Project&
 * @date     2026.02.19
 ****************************************************/
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

  if (ImGui::Checkbox("Snap to Grid", &Utils::appSettings.snapToGrid)) {
    project.isDirty = true;
    Utils::SaveSettings(Utils::appSettings);
  }
  ImGui::SameLine();
  if (ImGui::Checkbox("Snap to Objects", &Utils::appSettings.snapToObjects)) {
    project.isDirty = true;
    Utils::SaveSettings(Utils::appSettings);
  }
}

/*!***************************************************
 * @brief    Draws the CSV Data Source navigation panel
 * @param    project Project&
 * @date     2026.02.19
 ****************************************************/
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

/*!***************************************************
 * @brief    Draws the object hierarchy (tree) panel
 * @param    project Project&
 * @param    state InteractionState&
 * @date     2026.02.19
 ****************************************************/
void DrawObjectTree(Project &project, InteractionState &state) {
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

    ImGui::PushID((int)i);
    if (ImGui::Button(obj.isVisible ? "V" : "h", ImVec2(20, 0))) {
      state.PushHistory(project);
      obj.isVisible = !obj.isVisible;
    }
    if (ImGui::IsItemHovered())
      ImGui::SetTooltip(obj.isVisible ? "Visible" : "Hidden");

    ImGui::SameLine();
    if (ImGui::Button(obj.isLocked ? "L" : "u", ImVec2(20, 0))) {
      state.PushHistory(project);
      obj.isLocked = !obj.isLocked;
    }
    if (ImGui::IsItemHovered())
      ImGui::SetTooltip(obj.isLocked ? "Locked" : "Unlocked");

    ImGui::SameLine();

    bool isSelected = OBJECTS::IsObjectSelected(state.selectedIndices, i);
    if (ImGui::Selectable(id.c_str(), isSelected)) {
      if (ImGui::GetIO().KeyShift) {
        if (isSelected) {
          state.selectedIndices.erase(std::find(state.selectedIndices.begin(),
                                                state.selectedIndices.end(),
                                                (int)i));
        } else {
          state.selectedIndices.push_back((int)i);
        }
      } else {
        state.selectedIndices.clear();
        state.selectedIndices.push_back((int)i);
      }
    }
    ImGui::PopID();
  }
}

/*!***************************************************
 * @brief    Draws the object properties panel
 * @param    project Project&
 * @param    state InteractionState&
 * @param    uiState UIState&
 * @date     2026.02.19
 ****************************************************/
void DrawPropertiesPanel(Project &project, InteractionState &state,
                         UIState &uiState) {
  ImGui::Spacing();
  ImGui::Separator();

  int selectedIndex = OBJECTS::GetPrimarySelection(state.selectedIndices);
  if (selectedIndex >= 0 && selectedIndex < (int)project.objects.size()) {
    LabelObject &obj = project.objects[selectedIndex];
    ImGui::Text("Properties (%zu selected)", state.selectedIndices.size());

    if (ImGui::Checkbox("Visible", &obj.isVisible)) {
      state.PushHistory(project); // Push state AFTER toggle for simplicity
                                  // here, or we'd need pre-toggle state
      for (int idx : state.selectedIndices)
        project.objects[idx].isVisible = obj.isVisible;
      project.isDirty = true;
    }
    ImGui::SameLine();
    if (ImGui::Checkbox("Locked", &obj.isLocked)) {
      state.PushHistory(project);
      for (int idx : state.selectedIndices)
        project.objects[idx].isLocked = obj.isLocked;
      project.isDirty = true;
    }

    float currentX = obj.x;
    if (ImGui::DragFloat("X", &currentX)) {
      state.PushHistory(project);
      float delta = currentX - obj.x;
      for (int idx : state.selectedIndices) {
        if (!project.objects[idx].isLocked)
          project.objects[idx].x += delta;
      }
      project.isDirty = true;
    }
    if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(0) &&
        ImGui::GetIO().MouseDragMaxDistanceSqr[0] < 5.0f) {
      ImGui::SetKeyboardFocusHere(-1);
    }
    float currentY = obj.y;
    if (ImGui::DragFloat("Y", &currentY)) {
      state.PushHistory(project);
      float delta = currentY - obj.y;
      for (int idx : state.selectedIndices) {
        if (!project.objects[idx].isLocked)
          project.objects[idx].y += delta;
      }
      project.isDirty = true;
    }
    if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(0) &&
        ImGui::GetIO().MouseDragMaxDistanceSqr[0] < 5.0f) {
      ImGui::SetKeyboardFocusHere(-1);
    }

    float currentRot = obj.rotation;
    if (ImGui::SliderFloat("Rotation", &currentRot, 0.0f, 360.0f, "%.0f deg")) {
      state.PushHistory(project);
      for (int idx : state.selectedIndices) {
        if (!project.objects[idx].isLocked)
          project.objects[idx].rotation = currentRot;
      }
      project.isDirty = true;
    }
    if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(0) &&
        ImGui::GetIO().MouseDragMaxDistanceSqr[0] < 5.0f) {
      ImGui::SetKeyboardFocusHere(-1);
    }

    // Type-Specific Properties
    if (obj.type == ObjectType::Text || obj.type == ObjectType::Field) {
      if (ImGui::Button("Import Font...")) {
        auto selection =
            pfd::open_file("Select Font", ".", {"Font Files", "*.ttf *.otf"})
                .result();
        if (!selection.empty())
          AssetManager::Get().ImportFont(selection[0]);
      }
      float currentSize = obj.fontSize;
      if (ImGui::SliderFloat("Font Size", &currentSize, 10.0f, 100.0f)) {
        state.PushHistory(project);
        for (int idx : state.selectedIndices) {
          if (!project.objects[idx].isLocked &&
              (project.objects[idx].type == ObjectType::Text ||
               project.objects[idx].type == ObjectType::Field)) {
            project.objects[idx].fontSize = currentSize;
          }
        }
        project.isDirty = true;
      }
      if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(0) &&
          ImGui::GetIO().MouseDragMaxDistanceSqr[0] < 5.0f) {
        ImGui::SetKeyboardFocusHere(-1);
      }
      float currentWidth = obj.width;
      if (ImGui::DragFloat("Box Width", &currentWidth, 1.0f, 0.0f, 1000.0f,
                           "%.1f")) {
        state.PushHistory(project);
        for (int idx : state.selectedIndices) {
          if (!project.objects[idx].isLocked &&
              (project.objects[idx].type == ObjectType::Text ||
               project.objects[idx].type == ObjectType::Field)) {
            project.objects[idx].width = currentWidth;
          }
        }
        project.isDirty = true;
      }
      if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(0) &&
          ImGui::GetIO().MouseDragMaxDistanceSqr[0] < 5.0f) {
        ImGui::SetKeyboardFocusHere(-1);
      }
      if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Set > 0 to enable text wrapping");

      const auto &fontList = AssetManager::Get().GetFontList();
      std::string currentFont = obj.fontName.empty() ? "Default" : obj.fontName;
      if (ImGui::BeginCombo("Font", currentFont.c_str(),
                            ImGuiComboFlags_HeightLarge)) {
        if (ImGui::Selectable("Default", obj.fontName.empty())) {
          state.PushHistory(project);
          for (int idx : state.selectedIndices) {
            if (!project.objects[idx].isLocked &&
                (project.objects[idx].type == ObjectType::Text ||
                 project.objects[idx].type == ObjectType::Field)) {
              project.objects[idx].fontName = "";
            }
          }
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
              state.PushHistory(project);
              for (int idx : state.selectedIndices) {
                if (!project.objects[idx].isLocked &&
                    (project.objects[idx].type == ObjectType::Text ||
                     project.objects[idx].type == ObjectType::Field)) {
                  project.objects[idx].fontName = f.name;
                }
              }
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
              state.PushHistory(project);
              for (int idx : state.selectedIndices) {
                if (!project.objects[idx].isLocked &&
                    (project.objects[idx].type == ObjectType::Text ||
                     project.objects[idx].type == ObjectType::Field)) {
                  project.objects[idx].fontName = f.name;
                }
              }
              project.isDirty = true;
            }
          }
        }
        ImGui::EndCombo();
      }

      ImGui::Spacing();
      ImGui::Separator();
      if (ImGui::Checkbox("Auto-Increment", &obj.isAutoIncrement)) {
        state.PushHistory(project);
        for (int idx : state.selectedIndices) {
          if (project.objects[idx].type == ObjectType::Text ||
              project.objects[idx].type == ObjectType::Field) {
            project.objects[idx].isAutoIncrement = obj.isAutoIncrement;
          }
        }
        project.isDirty = true;
      }

      if (obj.isAutoIncrement) {
        ImGui::Indent();
        static char preBuf[64], sufBuf[64];
        strncpy(preBuf, obj.autoPrefix.c_str(), sizeof(preBuf));
        strncpy(sufBuf, obj.autoSuffix.c_str(), sizeof(sufBuf));

        if (ImGui::InputText("Prefix", preBuf, sizeof(preBuf))) {
          obj.autoPrefix = preBuf;
          project.isDirty = true;
        }
        if (ImGui::InputText("Suffix", sufBuf, sizeof(sufBuf))) {
          obj.autoSuffix = sufBuf;
          project.isDirty = true;
        }

        if (ImGui::DragInt("Start", &obj.autoStart, 1, 0, 1000000))
          project.isDirty = true;
        if (ImGui::DragInt("Step", &obj.autoStep, 1, -1000, 1000))
          project.isDirty = true;
        if (ImGui::DragInt("Current", &obj.autoCurrent, 1, 0, 1000000))
          project.isDirty = true;

        if (ImGui::Button("Reset to Start")) {
          state.PushHistory(project);
          obj.autoCurrent = obj.autoStart;
          project.isDirty = true;
        }
        ImGui::Unindent();
      }
    } else if (obj.type == ObjectType::Border ||
               obj.type == ObjectType::ShapeRect) {
      float currentThick = obj.fontSize;
      if (ImGui::SliderFloat("Thickness", &currentThick, 1, 20)) {
        state.PushHistory(project);
        for (int idx : state.selectedIndices) {
          if (!project.objects[idx].isLocked &&
              (project.objects[idx].type == ObjectType::Border ||
               project.objects[idx].type == ObjectType::ShapeRect)) {
            project.objects[idx].fontSize = currentThick;
          }
        }
        project.isDirty = true;
      }
      if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(0) &&
          ImGui::GetIO().MouseDragMaxDistanceSqr[0] < 5.0f) {
        ImGui::SetKeyboardFocusHere(-1);
      }

      float currentRadius = obj.cornerRadius;
      if (ImGui::SliderFloat("Radius", &currentRadius, 0, 50)) {
        state.PushHistory(project);
        for (int idx : state.selectedIndices) {
          if (!project.objects[idx].isLocked &&
              (project.objects[idx].type == ObjectType::Border ||
               project.objects[idx].type == ObjectType::ShapeRect)) {
            project.objects[idx].cornerRadius = currentRadius;
          }
        }
        project.isDirty = true;
      }
      if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(0) &&
          ImGui::GetIO().MouseDragMaxDistanceSqr[0] < 5.0f) {
        ImGui::SetKeyboardFocusHere(-1);
      }
      float currentW = obj.width;
      if (ImGui::DragFloat("W", &currentW)) {
        state.PushHistory(project);
        for (int idx : state.selectedIndices) {
          if (!project.objects[idx].isLocked &&
              (project.objects[idx].type == ObjectType::Border ||
               project.objects[idx].type == ObjectType::ShapeRect)) {
            project.objects[idx].width = currentW;
          }
        }
        project.isDirty = true;
      }
      if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(0) &&
          ImGui::GetIO().MouseDragMaxDistanceSqr[0] < 5.0f) {
        ImGui::SetKeyboardFocusHere(-1);
      }

      float currentH = obj.height;
      if (ImGui::DragFloat("H", &currentH)) {
        state.PushHistory(project);
        for (int idx : state.selectedIndices) {
          if (!project.objects[idx].isLocked &&
              (project.objects[idx].type == ObjectType::Border ||
               project.objects[idx].type == ObjectType::ShapeRect)) {
            project.objects[idx].height = currentH;
          }
        }
        project.isDirty = true;
      }
      if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(0) &&
          ImGui::GetIO().MouseDragMaxDistanceSqr[0] < 5.0f) {
        ImGui::SetKeyboardFocusHere(-1);
      }
    } else if (obj.type == ObjectType::ShapeCircle ||
               obj.type == ObjectType::Line) {
      if (ImGui::SliderFloat("Line Thickness", &obj.fontSize, 1.0f, 20.0f)) {
        state.PushHistory(project);
        project.isDirty = true;
      }
      if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(0) &&
          ImGui::GetIO().MouseDragMaxDistanceSqr[0] < 5.0f) {
        ImGui::SetKeyboardFocusHere(-1);
      }

      if (ImGui::DragFloat("Width", &obj.width)) {
        state.PushHistory(project);
        project.isDirty = true;
      }
      if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(0) &&
          ImGui::GetIO().MouseDragMaxDistanceSqr[0] < 5.0f) {
        ImGui::SetKeyboardFocusHere(-1);
      }

      if (ImGui::DragFloat("Height", &obj.height)) {
        state.PushHistory(project);
        project.isDirty = true;
      }
      if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(0) &&
          ImGui::GetIO().MouseDragMaxDistanceSqr[0] < 5.0f) {
        ImGui::SetKeyboardFocusHere(-1);
      }
    } else if (obj.type == ObjectType::QRCode) {
      if (ImGui::DragFloat("Size", &obj.width, 1.0f, 10.0f, 500.0f)) {
        state.PushHistory(project);
        obj.height = obj.width; // Keep Square
        project.isDirty = true;
      }
      if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(0) &&
          ImGui::GetIO().MouseDragMaxDistanceSqr[0] < 5.0f) {
        ImGui::SetKeyboardFocusHere(-1);
      }
    } else if (obj.type == ObjectType::Image) {
      int thresh = obj.threshold;
      if (ImGui::SliderInt("Threshold", &thresh, 0, 255)) {
        state.PushHistory(project); // Note: dragging might spam history,
                                    // ideally push on release
        for (int idx : state.selectedIndices) {
          if (!project.objects[idx].isLocked &&
              project.objects[idx].type == ObjectType::Image) {
            project.objects[idx].threshold = thresh;
          }
        }
        project.isDirty = true;
      }
      if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(0) &&
          ImGui::GetIO().MouseDragMaxDistanceSqr[0] < 5.0f) {
        ImGui::SetKeyboardFocusHere(-1);
      }

      if (ImGui::DragFloat("Width", &obj.width)) {
        state.PushHistory(project);
        project.isDirty = true;
      }
      if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(0) &&
          ImGui::GetIO().MouseDragMaxDistanceSqr[0] < 5.0f) {
        ImGui::SetKeyboardFocusHere(-1);
      }

      if (ImGui::DragFloat("Height", &obj.height)) {
        state.PushHistory(project);
        project.isDirty = true;
      }
      if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(0) &&
          ImGui::GetIO().MouseDragMaxDistanceSqr[0] < 5.0f) {
        ImGui::SetKeyboardFocusHere(-1);
      }
      ImGui::Spacing();
      if (ImGui::Button("Browse Image...")) {
        auto selection =
            pfd::open_file("Select Image", ".",
                           {"Image Files", "*.png *.jpg *.jpeg *.bmp"})
                .result();
        if (!selection.empty()) {
          state.PushHistory(project);
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
      if (ImGui::DragFloat("Width", &obj.width)) {
        state.PushHistory(project);
        project.isDirty = true;
      }
      if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(0) &&
          ImGui::GetIO().MouseDragMaxDistanceSqr[0] < 5.0f) {
        ImGui::SetKeyboardFocusHere(-1);
      }

      if (ImGui::DragFloat("Height", &obj.height)) {
        state.PushHistory(project);
        project.isDirty = true;
      }
      if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(0) &&
          ImGui::GetIO().MouseDragMaxDistanceSqr[0] < 5.0f) {
        ImGui::SetKeyboardFocusHere(-1);
      }
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
          state.PushHistory(project);
          for (int idx : state.selectedIndices) {
            if (!project.objects[idx].isLocked)
              project.objects[idx].linkedColumn = "";
          }
          project.isDirty = true;
        }
        for (const auto &header : project.csvHeaders) {
          bool isSelected = (obj.linkedColumn == header);
          if (ImGui::Selectable(header.c_str(), isSelected)) {
            state.PushHistory(project);
            for (int idx : state.selectedIndices) {
              if (!project.objects[idx].isLocked) {
                project.objects[idx].linkedColumn = header;
                if (!project.csvRows.empty()) {
                  for (size_t i = 0; i < project.csvHeaders.size(); i++) {
                    if (project.csvHeaders[i] == header) {
                      project.objects[idx].data = project.csvRows[0][i];
                      break;
                    }
                  }
                }
              }
            }
            project.isDirty = true;
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
      state.PushHistory(project);
      for (int idx : state.selectedIndices) {
        if (!project.objects[idx].isLocked)
          project.objects[idx].data = buffer;
      }
      project.isDirty = true;
    }
  }
}

/*************************************
 * Helper functions for menu sections
 *************************************/

/*!***************************************************
 * @brief    Draws the main menu bar and its submenus
 * @param    project Project&
 * @param    uiState UIState&
 * @date     2026.02.19
 ****************************************************/
void DrawFileMenu(Project &project, UIState &uiState) {
  const auto saveProject = [&project]() {
    if (project.projectFilePath.empty()) {
      return Utils::SaveProject(project);
    }
    return Utils::SaveProject(project, project.projectFilePath);
  };

  if (ImGui::MenuItem("Save", "Ctrl+S")) {
    if (saveProject()) {
      project.isDirty = false;
    }
  }

  if (ImGui::MenuItem("Save As...")) {
    if (Utils::SaveProject(project)) {
      project.isDirty = false;
    }
  }

  if (ImGui::MenuItem("Load Project", "Ctrl+L")) {
    uiState.triggerLoadConfirmation = true;
  }

  ImGui::Separator();

  if (ImGui::MenuItem("Export to PNG (Test Print)")) {
    const std::string exportPath = "test_label.png";
    Utils::ExportProjectToPNG(exportPath, project);
    Utils::OpenFile(exportPath);
  }

  ImGui::Separator();

  if (ImGui::MenuItem("Load CSV Data...")) {
    auto selection =
        pfd::open_file("Select CSV", ".", {"CSV Files", "*.csv"}).result();
    if (!selection.empty()) {
      Utils::LoadCSV(selection[0], project);
      project.currentCSVRow = 0;
      Utils::ApplyCSVDataToObjects(project);
    }
  }

  if (ImGui::MenuItem("Batch Print (CSV)", "Ctrl+Shift+B")) {
    if (project.csvRows.empty()) {
      std::cout << "[UI] No CSV loaded. Cannot batch print." << std::endl;
    } else {
      uiState.triggerBatchPopup = true;
    }
  }

  if (ImGui::MenuItem("Sequence Print (Auto-Inc)", "Ctrl+Shift+P")) {
    uiState.triggerSequencePopup = true;
  }

  ImGui::Separator();

  if (ImGui::MenuItem("Exit", "Alt+F4")) {
    RequestExit(uiState);
  }
}

/*!***************************************************
 * @brief    Draws the Edit menu with undo/redo and clipboard operations
 * @param    project Project&
 * @param    uiState UIState&
 * @param    state InteractionState&
 * @date     2026.02.19
 ****************************************************/
void DrawEditMenu(Project &project, UIState &uiState, InteractionState &state) {
  const bool hasSelection = !state.selectedIndices.empty();
  const bool hasClipboard = !state.clipboard.empty();
  const bool canUndo = !state.undoStack.empty();
  const bool canRedo = !state.redoStack.empty();

  if (ImGui::MenuItem("Undo", "Ctrl+Z", false, canUndo)) {
    state.Undo(project);
    state.selectedIndices.clear();
  }

  if (ImGui::MenuItem("Redo", "Ctrl+Y", false, canRedo)) {
    state.Redo(project);
    state.selectedIndices.clear();
  }

  ImGui::Separator();

  if (ImGui::MenuItem("Cut", "Ctrl+X", false, hasSelection)) {
    PerformCutOperation(project, state);
  }

  if (ImGui::MenuItem("Copy", "Ctrl+C", false, hasSelection)) {
    PerformCopyOperation(project, state);
  }

  if (ImGui::MenuItem("Paste", "Ctrl+V", false, hasClipboard)) {
    PerformPasteOperation(project, state);
  }

  ImGui::Separator();

  if (ImGui::MenuItem("Delete", "Del", false, hasSelection)) {
    PerformDeleteOperation(project, state);
  }
}

/*!***************************************************
 * @brief    Draws the Assets menu for managing icons and borders
 * @param    uiState UIState&
 * @date     2026.02.19
 ****************************************************/
void DrawAssetsMenu(UIState &uiState) {
  if (ImGui::MenuItem("Manage Icon Library")) {
    uiState.triggerLibraryManager = true;
  }

  if (ImGui::MenuItem("Manage Deco Borders")) {
    uiState.triggerBorderManager = true;
  }
}

/*!***************************************************
 * @brief    Draws the Printer menu for managing printer connection and printing
 * @param    project Project&
 * @param    uiState UIState&
 * @date     2026.02.19
 ****************************************************/
void DrawPrinterMenu(Project &project, UIState &uiState) {
  auto &printer = Printer::Get();

  if (printer.IsConnected()) {
    ImGui::TextColored(ImVec4(0, 1, 0, 1), "Connected: %s",
                       printer.GetConnectedName().c_str());

    if (ImGui::MenuItem("Disconnect")) {
      printer.Disconnect();
    }

    ImGui::Separator();

    if (ImGui::MenuItem("Print Single Label", "Ctrl+P")) {
      Protocol::PrintLabel(project);
    }
  } else {
    ImGui::TextColored(ImVec4(1, 0, 0, 1), "Status: Disconnected");
  }

  ImGui::Separator();

  if (ImGui::MenuItem("Scan for Devices")) {
    printer.StartScan();
    uiState.triggerScanPopup = true;
  }
}

/*!***************************************************
 * @brief    Draws the Help menu with links to documentation and about dialog
 * @param    uiState UIState&
 * @date     2026.02.19
 ****************************************************/
void DrawHelpMenu(UIState &uiState) {
  if (ImGui::MenuItem("User Guide")) {
    Utils::OpenFile("https://bearded-griffin.github.io/Desktop-D30/");
  }

  ImGui::Separator();

  if (ImGui::MenuItem("About")) {
    uiState.showAboutDialog = true;
  }
}

/*!***************************************************
 * @brief    Draws all active popups based on UIState flags
 * @param    project Project&
 * @param    uiState UIState&
 * @param    state InteractionState&
 * @date     2026.02.19
 ****************************************************/
void DrawPopups(Project &project, UIState &uiState, InteractionState &state) {
  DrawDeviceScanPopup(uiState);
  DrawBatchPrintPopup(project, uiState);
  DrawSequencePrintPopup(project, uiState);
  DrawIconLibraryPopup(project, uiState, state);
  DrawBorderLibraryPopup(project, uiState, state);
  DrawLibraryManager(uiState);
  DrawBorderManager(uiState);
}
/***************************************
 * Helper functions for edit operations
 ***************************************/

/*!***************************************************
 * @brief    Performs the cut operation on selected objects
 * @param    project Project&
 * @param    state InteractionState&
 * @date     2026.02.19
 ****************************************************/
void PerformCutOperation(Project &project, InteractionState &state) {
  state.PushHistory(project);
  state.clipboard.clear();

  std::vector<int> sortedIndices = state.selectedIndices;
  std::sort(sortedIndices.begin(), sortedIndices.end(), std::greater<int>());

  for (int idx : sortedIndices) {
    state.clipboard.push_back(project.objects[idx]);
    project.objects.erase(project.objects.begin() + idx);
  }

  state.selectedIndices.clear();
  project.isDirty = true;
}

/*!***************************************************
 * @brief    Performs the copy operation on selected objects
 * @param    project Project&
 * @param    state InteractionState&
 * @date     2026.02.19
 ****************************************************/
void PerformCopyOperation(Project &project, InteractionState &state) {
  state.clipboard.clear();
  state.clipboard.reserve(state.selectedIndices.size());

  for (int idx : state.selectedIndices) {
    state.clipboard.push_back(project.objects[idx]);
  }
}

/*!***************************************************
 * @brief    Performs the paste operation from clipboard
 * @param    project Project&
 * @param    state InteractionState&
 * @date     2026.02.19
 ****************************************************/
void PerformPasteOperation(Project &project, InteractionState &state) {
  state.PushHistory(project);
  state.selectedIndices.clear();
  state.selectedIndices.reserve(state.clipboard.size());

  const int offset = 10;
  for (const auto &obj : state.clipboard) {
    LabelObject newObj = obj;
    newObj.x += offset;
    newObj.y += offset;
    project.objects.push_back(newObj);
    state.selectedIndices.push_back(static_cast<int>(project.objects.size()) -
                                    1);
  }

  project.isDirty = true;
}

/*!***************************************************
 * @brief    Performs the delete operation on selected objects
 * @param    project Project&
 * @param    state InteractionState&
 * @date     2026.02.19
 ****************************************************/
void PerformDeleteOperation(Project &project, InteractionState &state) {
  state.PushHistory(project);

  std::vector<int> sortedIndices = state.selectedIndices;
  std::sort(sortedIndices.begin(), sortedIndices.end(), std::greater<int>());

  for (int idx : sortedIndices) {
    auto &objToDelete = project.objects[idx];
    if (objToDelete.texture.id != 0) {
      UnloadTexture(objToDelete.texture);
    }
    project.objects.erase(project.objects.begin() + idx);
  }

  state.selectedIndices.clear();
  project.isDirty = true;
}

} // Anonymous namespace

/*!***************************************************
 * @brief    Initializes the UI system
 * @details
 * @return   void
 * @note
 * @date     2026.02.01
 ****************************************************/
void InitializeUI() {
  SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
  InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Desktop-D30");

  rlImGuiSetup(true);
}

/*!***************************************************
 * @brief    Updates the window title
 * @details
 * @param    project const Project&
 * @return   void
 * @note
 * @date     2026.02.01
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
 ****************************************************/
void RequestExit(UIState &uiState) { uiState.exitRequested = true; }

/*!***************************************************
 * @brief    Checks if application should close
 * @details
 * @return   bool
 * @note
 * @date     2026.02.03
 ****************************************************/
bool ShouldClose(const UIState &uiState) { return uiState.forceQuit; }

/*!***************************************************
 * @brief    Clears the exit request flag
 * @details
 * @return   void
 * @note
 * @date     2026.02.03
 ****************************************************/
void ClearExitRequest(UIState &uiState) { uiState.exitRequested = false; }

/*!***************************************************
 * @brief    Draws the exit confirmation dialog
 * @details
 * @param    project Project&
 * @return   void
 * @note
 * @date     2026.02.03
 ****************************************************/
void DrawExitConfirmation(Project &project, UIState &uiState) {

  // Handle exit request
  if (uiState.exitRequested) {
    if (!project.isDirty) {
      uiState.forceQuit = true;
    } else {
      ImGui::OpenPopup(ExitDialog::TITLE);
    }
  }

  // Draw confirmation modal
  if (ImGui::BeginPopupModal(ExitDialog::TITLE, nullptr,
                             ImGuiWindowFlags_AlwaysAutoResize)) {
    ImGui::Text(ExitDialog::MESSAGE);
    ImGui::Separator();

    // Save and Exit button
    if (ImGui::Button(ExitDialog::SAVE_BUTTON, ExitDialog::BUTTON_SIZE)) {
      bool saveSuccess =
          project.projectFilePath.empty()
              ? Utils::SaveProject(project)
              : Utils::SaveProject(project, project.projectFilePath);

      if (saveSuccess) {
        project.isDirty = false;
        uiState.forceQuit = true;
      }
      ImGui::CloseCurrentPopup();
    }
    ImGui::SetItemDefaultFocus();
    ImGui::SameLine();

    // Exit Without Saving button
    if (ImGui::Button(ExitDialog::DISCARD_BUTTON,
                      ExitDialog::WIDE_BUTTON_SIZE)) {
      uiState.forceQuit = true;
      ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();

    // Cancel button
    if (ImGui::Button(ExitDialog::CANCEL_BUTTON, ExitDialog::BUTTON_SIZE)) {
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
 ****************************************************/
void DrawLoadConfirmation(Project &project, UIState &uiState) {
  // Constants for consistent UI sizing
  constexpr ImVec2 BUTTON_SIZE(120, 0);
  constexpr const char *DEFAULT_PROJECT_FILE = "project.d30";

  // Helper lambda for loading project
  auto LoadProject = [&]() -> bool {
    if (Utils::LoadProject(DEFAULT_PROJECT_FILE, project)) {
      project.isDirty = false;
      return true;
    }
    return false;
  };

  // Helper lambda for saving project
  auto SaveProject = [&]() -> bool {
    return project.projectFilePath.empty()
               ? Utils::SaveProject(project)
               : Utils::SaveProject(project, project.projectFilePath);
  };

  // Handle load confirmation trigger
  if (uiState.triggerLoadConfirmation) {
    if (!project.isDirty) {
      LoadProject();
    } else {
      ImGui::OpenPopup("ConfirmLoad");
    }
    uiState.triggerLoadConfirmation = false;
  }

  // Draw confirmation modal
  if (ImGui::BeginPopupModal("ConfirmLoad", nullptr,
                             ImGuiWindowFlags_AlwaysAutoResize)) {
    ImGui::Text("You have unsaved changes! Do you want to save before "
                "loading a new project?\n\n");
    ImGui::Separator();

    // Save and Load button
    if (ImGui::Button("Save and Load", BUTTON_SIZE)) {
      if (SaveProject() && LoadProject()) {
        ImGui::CloseCurrentPopup();
      }
    }
    ImGui::SameLine();

    // Discard and Load button
    if (ImGui::Button("Discard and Load", BUTTON_SIZE)) {
      if (LoadProject()) {
        ImGui::CloseCurrentPopup();
      }
    }
    ImGui::SameLine();

    // Cancel button
    if (ImGui::Button("Cancel", BUTTON_SIZE)) {
      ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
  }
}

/*!***************************************************
 * @brief    Draws the loading splash screen
 * @details  Renders a logo and loading progress bar.
 * @return   void
 * @date     2026.02.19
 ****************************************************/
void DrawSplashScreen() {
  // Constants for consistent UI styling

  // Get screen dimensions
  const int screenW = GetScreenWidth();
  const int screenH = GetScreenHeight();

  // Clear background
  ClearBackground(Colors::Background);

  // Get font (could be cached if called frequently)
  const Font font = AssetManager::Get().GetDefaultFont();

  // Helper lambda for drawing centered text
  auto DrawCenteredText = [&](const char *text, float fontSize, float y,
                              Color color) {
    const Vector2 textSize =
        MeasureTextEx(font, text, fontSize, Layout::TextSpacing);
    const float x = (screenW - textSize.x) / 2;
    DrawTextEx(font, text, {x, y}, fontSize, Layout::TextSpacing, color);
  };

  // Draw title
  DrawCenteredText("Desktop-D30", Layout::TitleFontSize,
                   screenH / 2 - Layout::TitleOffsetY, Colors::Title);

  // Get and draw progress bar
  const float progress = AssetManager::Get().GetLoadProgress();
  const int barX = (screenW - Layout::ProgressBarWidth) / 2;
  const int barY = screenH / 2 + Layout::ProgressBarOffsetY;

  DrawRectangleLines(barX, barY, Layout::ProgressBarWidth,
                     Layout::ProgressBarHeight, Colors::ProgressBarBorder);
  DrawRectangle(barX + 2, barY + 2,
                static_cast<int>((Layout::ProgressBarWidth - 4) * progress),
                Layout::ProgressBarHeight - 4, Colors::ProgressBar);

  // Determine and draw status message
  const char *statusMsg = "Loading Icons...";
  if (progress > 0.9f)
    statusMsg = "Finalizing Graphics...";
  else if (progress > 0.6f)
    statusMsg = "Polishing the Pixels...";
  else if (progress > 0.3f)
    statusMsg = "Wrangling the Angry Pixels...";

  DrawCenteredText(statusMsg, Layout::StatusFontSize,
                   barY - Layout::StatusOffsetY, Colors::StatusText);

  // Draw percentage text
  const int percent = static_cast<int>(progress * 100);
  char percentStr[8]; // Enough for "100%" + null terminator
  snprintf(percentStr, sizeof(percentStr), "%d%%", percent);

  DrawCenteredText(percentStr, Layout::PercentFontSize,
                   barY + Layout::ProgressBarHeight + Layout::PercentOffsetY,
                   Colors::PercentText);
}

/*!***************************************************
 * @brief    Draw the main menu
 * @details  Draws the drop down main menu that
 * contains all the options for the program.
 * @param    project Project&
 * @return   void
 * @note
 * @date     2026.01.19
 ****************************************************/
void DrawMainMenu(Project &project, UIState &uiState, InteractionState &state) {
  if (!ImGui::BeginMainMenuBar()) {
    return;
  }

  // File Menu
  if (ImGui::BeginMenu("File")) {
    DrawFileMenu(project, uiState);
    ImGui::EndMenu();
  }

  // Edit Menu
  if (ImGui::BeginMenu("Edit")) {
    DrawEditMenu(project, uiState, state);
    ImGui::EndMenu();
  }

  // Assets Menu
  if (ImGui::BeginMenu("Assets")) {
    DrawAssetsMenu(uiState);
    ImGui::EndMenu();
  }

  // Printer Menu
  if (ImGui::BeginMenu("Printer")) {
    DrawPrinterMenu(project, uiState);
    ImGui::EndMenu();
  }

  // Help Menu
  if (ImGui::BeginMenu("Help")) {
    DrawHelpMenu(uiState);
    ImGui::EndMenu();
  }

  ImGui::EndMainMenuBar();

  // Draw Popups
  DrawPopups(project, uiState, state);
}

/*!***************************************************
 * @brief    Draws the side bar
 * @details  It draws the side "Inspector" bar on the left side.
 * @param    project Project&
 * @param    selectedIndices std::vector<int>&
 * @return   void
 * @note
 * @date     2026.01.19
 ****************************************************/
void DrawSidebar(Project &project, InteractionState &state, UIState &uiState) {
  // Constants for consistent UI sizing
  constexpr ImVec2 ALIGN_BTN_SIZE(40, 0);
  constexpr ImVec2 DIST_BTN_SIZE(60, 0);
  constexpr ImVec2 TOOL_BTN_SIZE(110, 0);
  constexpr int MAX_IMPORT_SIZE = 512;
  constexpr int MIN_DISTRIBUTE_COUNT = 3;

  // Helper lambda for creating buttons with tooltips
  auto CreateButtonWithTooltip = [](const char *label, const ImVec2 &size,
                                    const char *tooltip) {
    bool clicked = ImGui::Button(label, size);
    if (ImGui::IsItemHovered() && tooltip) {
      ImGui::SetTooltip(tooltip);
    }
    return clicked;
  };

  // Helper lambda for alignment operations
  auto CreateAlignmentButton = [&](const char *label, AlignmentType type) {
    if (ImGui::Button(label, ALIGN_BTN_SIZE)) {
      state.PushHistory(project);
      OBJECTS::AlignObjects(project, state.selectedIndices, type,
                            uiState.alignToCanvas);
    }
  };

  ImGui::Begin("Inspector", nullptr,
               ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);
  ImGui::SetWindowPos({0, 20}, ImGuiCond_FirstUseEver);
  ImGui::SetWindowSize({300, 600}, ImGuiCond_FirstUseEver);

  // Main UI sections
  DrawProjectSettings(project);
  DrawDataSource(project);
  DrawObjectTree(project, state);
  DrawPropertiesPanel(project, state, uiState);

  // Alignment Tools Section
  if (!state.selectedIndices.empty()) {
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Text("Alignment");
    ImGui::Checkbox("Align to Canvas", &uiState.alignToCanvas);

    // Horizontal alignment buttons
    CreateAlignmentButton("L", ALIGN_LEFT);
    ImGui::SameLine();
    CreateAlignmentButton("CH", ALIGN_CENTER_H);
    ImGui::SameLine();
    CreateAlignmentButton("R", ALIGN_RIGHT);

    // Vertical alignment buttons
    CreateAlignmentButton("T", ALIGN_TOP);
    ImGui::SameLine();
    CreateAlignmentButton("CV", ALIGN_CENTER_V);
    ImGui::SameLine();
    CreateAlignmentButton("B", ALIGN_BOTTOM);

    // Distribution buttons (only when enough objects selected)
    if (state.selectedIndices.size() >= MIN_DISTRIBUTE_COUNT) {
      if (ImGui::Button("Dist H", DIST_BTN_SIZE)) {
        state.PushHistory(project);
        OBJECTS::DistributeObjects(project, state.selectedIndices,
                                   DISTRIBUTE_HORIZONTALLY);
      }
      ImGui::SameLine();
      if (ImGui::Button("Dist V", DIST_BTN_SIZE)) {
        state.PushHistory(project);
        OBJECTS::DistributeObjects(project, state.selectedIndices,
                                   DISTRIBUTE_VERTICALLY);
      }
    }
  }

  // Tools Section
  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Text("Tools");

  // Basic tools
  if (CreateButtonWithTooltip("Add Text", TOOL_BTN_SIZE, "Alt+T")) {
    OBJECTS::AddTextObject(project, state);
  }
  ImGui::SameLine();
  if (CreateButtonWithTooltip("Add Field", TOOL_BTN_SIZE, "Alt+F")) {
    OBJECTS::AddFieldObject(project, state);
  }

  // Media tools
  if (CreateButtonWithTooltip("Add QR", TOOL_BTN_SIZE, "Alt+Q")) {
    OBJECTS::AddQRCodeObject(project, state);
  }
  ImGui::SameLine();
  if (CreateButtonWithTooltip("Add Barcode", TOOL_BTN_SIZE, "Alt+B")) {
    OBJECTS::AddBarcodeObject(project, state);
  }

  // Graphics tools
  if (CreateButtonWithTooltip("Add Image", TOOL_BTN_SIZE, nullptr)) {
    auto selection = pfd::open_file("Select Image", ".",
                                    {"Image Files", "*.png *.jpg *.jpeg *.bmp"})
                         .result();
    if (!selection.empty()) {
      state.PushHistory(project);
      LabelObject obj = OBJECTS::CreateImageObject(0, 0, 60, 60, selection[0]);

      // Load and process image
      Image img = LoadImage(obj.data.c_str());
      if (img.data != nullptr) {
        // Calculate new dimensions maintaining aspect ratio
        float aspect = static_cast<float>(img.width) / img.height;
        int newWidth = img.width;
        int newHeight = img.height;

        if (img.width > MAX_IMPORT_SIZE || img.height > MAX_IMPORT_SIZE) {
          if (img.width > img.height) {
            newWidth = MAX_IMPORT_SIZE;
            newHeight = static_cast<int>(MAX_IMPORT_SIZE / aspect);
          } else {
            newHeight = MAX_IMPORT_SIZE;
            newWidth = static_cast<int>(MAX_IMPORT_SIZE * aspect);
          }
          ImageResize(&img, newWidth, newHeight);
        }

        obj.width = static_cast<float>(newWidth);
        obj.height = static_cast<float>(newHeight);

        // Add object to project
        project.objects.push_back(obj);
        project.isDirty = true;
        state.selectedIndices.clear();
        state.selectedIndices.push_back(
            static_cast<int>(project.objects.size() - 1));

        // Load texture and cleanup
        project.objects[state.selectedIndices.back()].texture =
            LoadTextureFromImage(img);
        UnloadImage(img);
      }
    }
  }
  ImGui::SameLine();
  if (ImGui::Button("Add Icon", TOOL_BTN_SIZE)) {
    uiState.triggerIconPopup = true;
  }

  // Shape tools
  if (CreateButtonWithTooltip("Add Line", TOOL_BTN_SIZE, "Alt+L")) {
    OBJECTS::AddLineObject(project, state);
  }
  ImGui::SameLine();
  if (CreateButtonWithTooltip("Add Rect", TOOL_BTN_SIZE, "Alt+R")) {
    OBJECTS::AddRectangleObject(project, state);
  }

  if (CreateButtonWithTooltip("Add Circle", TOOL_BTN_SIZE, "Alt+C")) {
    OBJECTS::AddCircleObject(project, state);
  }

  // Decor tools
  ImGui::SameLine();
  if (CreateButtonWithTooltip("Add Border", TOOL_BTN_SIZE, "Alt+D")) {
    OBJECTS::AddBorderObject(project, state);
  }

  if (ImGui::Button("Deco Border", TOOL_BTN_SIZE)) {
    uiState.triggerBorderPopup = true;
  }

  ImGui::End();
}

/*!***************************************************
 * @brief    Draws the About dialog
 * @param    uiState UIState&
 * @date     2026.02.19
 ****************************************************/
void DrawAboutDialog(UIState &uiState) {

  // Open dialog if triggered
  if (uiState.showAboutDialog) {
    ImGui::OpenPopup(AboutDialog::TITLE);
  }

  // Draw dialog content
  if (ImGui::BeginPopupModal(AboutDialog::TITLE, &uiState.showAboutDialog,
                             ImGuiWindowFlags_AlwaysAutoResize)) {
    // Application information
    ImGui::Text(AboutDialog::APP_NAME);
    ImGui::Text(AboutDialog::VERSION);
    ImGui::Separator();
    ImGui::Text(AboutDialog::AUTHOR);
    ImGui::Text(AboutDialog::LICENSE);

    // Support section
    ImGui::Spacing();
    ImGui::Text(AboutDialog::SUPPORT_TEXT);
    if (ImGui::Button(AboutDialog::SUPPORT_BUTTON)) {
      Utils::OpenFile(AboutDialog::SUPPORT_URL);
    }

    // Close button
    ImGui::Separator();
    if (ImGui::Button(AboutDialog::CLOSE_BUTTON, AboutDialog::BUTTON_SIZE)) {
      uiState.showAboutDialog = false;
      ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
  }
}

/*!***************************************************
 * @brief    Main UI draw call
 * @param    project Project&
 * @param    state InteractionState&
 * @param    uiState UIState&
 * @date     2026.02.19
 ****************************************************/
void Draw(Project &project, InteractionState &state, UIState &uiState) {
  DrawMainMenu(project, uiState, state);
  DrawSidebar(project, state, uiState);
  DrawExitConfirmation(project, uiState);
  DrawLoadConfirmation(project, uiState);
  DrawAboutDialog(uiState);
}

/*!***************************************************
 * @brief    Cleans up the UI system
 * @details
 * @param    currentProject Project&
 * @return   void
 * @note
 * @date     2026.02.03
 ****************************************************/
void CleanupApplication(Project &currentProject) {

  // Helper lambda for logging cleanup operations
  auto LogCleanup = [](const char *operation) {
    if constexpr (Cleanup::LOG_OPERATIONS) {
      std::cout << Cleanup::LOG_PREFIX << " " << operation << std::endl;
    }
  };

  // Cleanup sequence (order matters due to dependencies)
  try {
    // 1. Unload project objects first as they depend on assets
    LogCleanup("Unloading project objects...");
    OBJECTS::UnloadProjectObjects(currentProject);

    // 2. Unload assets after project objects no longer reference them
    LogCleanup("Unloading assets...");
    AssetManager::Get().UnloadAssets();

    // 3. Shutdown ImGui system after all UI elements are gone
    LogCleanup("Shutting down ImGui...");
    rlImGuiShutdown();

    // 4. Close window last as other systems might need window context
    LogCleanup("Closing window...");
    CloseWindow();

    LogCleanup("Application cleanup completed successfully");
  } catch (const std::exception &e) {
    std::cerr << Cleanup::LOG_PREFIX << " Error during cleanup: " << e.what()
              << std::endl;
    // Attempt to close window even if other cleanup fails
    try {
      CloseWindow();
    } catch (...) {
      std::cerr << Cleanup::LOG_PREFIX << " Critical: Failed to close window"
                << std::endl;
    }
  }
}

} // namespace UI