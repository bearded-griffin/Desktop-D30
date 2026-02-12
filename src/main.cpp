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
 * @file     main.cpp
 * @brief    The main entry point for Desktop-D30
 * @details  Handels the input from the user and draws the interface.
 * @note     Updated with Camera Centering, Object Clamping, and Deletion.
 * @date     2026.01.20
 * @author   bearded.griffin
 ****************************************************/

#include "camera.h"
#include "input.h"
#include "rendering.h"
#include "ui.h"
#include "utils.h"
#include "assets.h"
#include "rlImGui.h"

int main() {
  Camera2D camera;
  Project currentProject;
  InteractionState state;
  UI::UIState uiState;

  UI::InitializeUI();

  Utils::LoadSettings(Utils::appSettings);

  // --- SPLASH SCREEN LOADING LOOP ---
  AssetManager::Get().InitializeLoadQueue();
  
  bool loading = true;
  while (!WindowShouldClose() && loading) {
      // Process a small batch of icons per frame
      // 10 icons per frame is a good balance between load speed and framerate
      loading = AssetManager::Get().ProcessLoadQueue(10);
      
      BeginDrawing();
      UI::DrawSplashScreen();
      EndDrawing();
  }

  CAMERA::InitializeCamera(&SCREEN_WIDTH, &SCREEN_HEIGHT, &camera,
                           &currentProject);

  while (!UI::ShouldClose(uiState)) {
    if (WindowShouldClose()) {
      UI::RequestExit(uiState);
    }
    UI::UpdateWindowTitle(currentProject);
    CAMERA::UpdateCamera(camera, currentProject);

    BeginDrawing();
    ClearBackground(Utils::appSettings.darkTheme ? Color{40, 40, 40, 255} : RAYWHITE);

    rlImGuiBegin();
    INPUT::HandleInput(currentProject, state, camera);
    RENDERING::RenderScene(currentProject, state, camera, state.selectedIndex);
    UI::Draw(currentProject, state.selectedIndex, uiState);
    rlImGuiEnd();

    EndDrawing();
  }

  Utils::SaveSettings(Utils::appSettings);
  UI::CleanupApplication(currentProject);
  return 0;
}