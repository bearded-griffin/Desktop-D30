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
 * @file     include/ui.h
 * @brief    Handels all UI operations
 * @details  draws the menus, and canvas.
 * @note
 * @date     2026.01.19
 * @author   bearded.griffin
 ****************************************************/

#pragma once
#include "types.h"

namespace UI {

struct UIState {
  bool triggerIconPopup = false;
  bool triggerBorderPopup = false;
  bool triggerScanPopup = false;
  bool triggerBatchPopup = false;
  bool triggerLoadConfirmation = false;
  bool triggerLibraryManager = false;
  bool triggerBorderManager = false;
  bool alignToCanvas = false;
  bool exitRequested = false;
  bool forceQuit = false;
};

void DrawSidebar(Project &project, std::vector<int> &selectedIndices, UIState &uiState);
void DrawMainMenu(Project &project, UIState &uiState, std::vector<int> &selectedIndices);
void Draw(Project &project, std::vector<int> &selectedIndices, UIState &uiState);
void RequestExit(UIState &uiState);
bool ShouldClose(const UIState &uiState);
void ClearExitRequest(UIState &uiState);
void DrawExitConfirmation(Project &project, UIState &uiState);
void DrawLoadConfirmation(Project &project, UIState &uiState);
void DrawSplashScreen(); // New!
void InitializeUI();
void UpdateWindowTitle(const Project &project);
void CleanupApplication(Project &project);

void LoadFontPreviews();

} // namespace UI