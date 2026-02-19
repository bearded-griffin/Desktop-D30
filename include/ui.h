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
 * @file     include/ui.h
 * @brief    Handels all UI operations
 * @details  draws the menus, and canvas.
 * @note
 * @date     2026.01.19
 ****************************************************/

#pragma once
#include "types.h"

namespace UI {

struct UIState {
  bool triggerIconPopup = false;
  bool triggerBorderPopup = false;
  bool triggerScanPopup = false;
  bool triggerBatchPopup = false;
  bool triggerSequencePopup = false;
  bool triggerLoadConfirmation = false;
  bool triggerLibraryManager = false;
  bool triggerBorderManager = false;
  bool alignToCanvas = false;
  bool exitRequested = false;
  bool forceQuit = false;
  bool showAboutDialog = false;
};

void DrawSidebar(Project &project, InteractionState &state, UIState &uiState);
void DrawMainMenu(Project &project, UIState &uiState, InteractionState &state);
void DrawAboutDialog(UIState &uiState);
void Draw(Project &project, InteractionState &state, UIState &uiState);
void RequestExit(UIState &uiState);
bool ShouldClose(const UIState &uiState);
void ClearExitRequest(UIState &uiState);
void DrawExitConfirmation(Project &project, UIState &uiState);
void DrawLoadConfirmation(Project &project, UIState &uiState);
void DrawSplashScreen(); // New!
void InitializeUI();
void UpdateWindowTitle(const Project &project);
void CleanupApplication(Project &project);
} // namespace UI