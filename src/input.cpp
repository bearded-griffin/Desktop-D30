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
 * @file     src/input.cpp
 * @brief    Handles all Input
 * @details  Handles the mouse and keyboard interactions
 * with designing a label
 * @note
 * @date     2026.02.03
 * @author   bearded.griffin
 ****************************************************/

#include "input.h"
#include "assets.h"
#include "imgui.h"
#include "objects.h"
#include "protocol.h"
#include "utils.h"
#include <algorithm>

namespace INPUT_HANDLER {

/*!***************************************************
 * @brief    Handles Mouse Interactions
 * @details  Deals with mouse clicks, drags, and
 * resizing of objects on the canvas.
 * @param    project Project&
 * @param    state InteractionState&
 * @param    mouseWorld Vector2&
 * @param    camera Camera2D&
 * @return   void
 * @note
 * @date     2026.01.19
 * @author   bearded.griffin
 ****************************************************/
void HandleMouseInteractions(Project &project, InteractionState &state,
                             const Vector2 &mouseWorld,
                             const Camera2D &camera) {
  static Project preActionState;
  static bool potentialChange = false;

  if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
    preActionState = project;
    potentialChange = false;
    state.isResizing = false;
    state.activeHandle = HANDLE_NONE;

    int primaryIdx = OBJECTS::GetPrimarySelection(state.selectedIndices);
    if (primaryIdx != -1) {
      const auto &obj = project.objects[primaryIdx];
      if (obj.type == ObjectType::Line) {
        float handleRadius = HANDLE_RADIUS / camera.zoom;
        Vector2 start = {obj.x, obj.y};
        Vector2 end = {obj.x + obj.width, obj.y + obj.height};

        if (CheckCollisionPointCircle(mouseWorld, start, handleRadius)) {
          state.isResizing = true;
          state.activeHandle = HANDLE_TOP_LEFT;
        } else if (CheckCollisionPointCircle(mouseWorld, end, handleRadius)) {
          state.isResizing = true;
          state.activeHandle = HANDLE_BOTTOM_RIGHT;
        }
      } else {
        // Rotated handle detection
        Vector2 p = {mouseWorld.x - obj.x, mouseWorld.y - obj.y};
        p = Vector2Rotate(p, -obj.rotation * DEG2RAD);

        Rectangle localBounds = {0, 0, obj.width, obj.height};
        if (obj.type == ObjectType::Text || obj.type == ObjectType::Field) {
            if (localBounds.height <= 0) localBounds.height = obj.fontSize * 1.5f;
            if (localBounds.width <= 0) {
                Font f = AssetManager::Get().GetFont(obj.fontName);
                localBounds.width = MeasureTextEx(f, obj.data.c_str(), obj.fontSize, 2.0f).x;
            }
        }

        float handleRadius = HANDLE_RADIUS / camera.zoom;
        Vector2 localHandles[] = {
            {0, 0},                                     // Top-Left
            {localBounds.width, 0},                     // Top-Right
            {0, localBounds.height},                    // Bottom-Left
            {localBounds.width, localBounds.height}     // Bottom-Right
        };

        for (int i = 0; i < 4; i++) {
          if (CheckCollisionPointCircle(p, localHandles[i], handleRadius)) {
            if (i == 0) { // Top-Left: Delete
              state.PushHistory(project);
              auto &objToDelete = project.objects[primaryIdx];
              if (objToDelete.texture.id != 0) UnloadTexture(objToDelete.texture);
              project.objects.erase(project.objects.begin() + primaryIdx);
              project.isDirty = true;
              
              state.selectedIndices.clear();
              state.isDraggingObject = false;
              return; 
            }
            state.isResizing = true;
            state.activeHandle = static_cast<ResizeHandle>(i + 1);
            break;
          }
        }
      }
    }

    if (!state.isResizing) {
      OBJECTS::HandleObjectSelection(project, state.selectedIndices,
                                     state.isDraggingObject, state.dragOffset,
                                     mouseWorld, camera);
    }
  }

  if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
    if ((state.isDraggingObject || state.isResizing) && potentialChange) {
      state.PushHistory(preActionState);
    }
    state.isDraggingObject = false;
    state.isResizing = false;
    state.activeHandle = HANDLE_NONE;
    state.activeGuides.clear();
    SetMouseCursor(MOUSE_CURSOR_DEFAULT);
  }

  if (state.isResizing && !state.selectedIndices.empty()) {
    OBJECTS::HandleObjectResize(project, state.selectedIndices.back(),
                                state.activeHandle, mouseWorld, camera);
    potentialChange = true;
  } else if (state.isDraggingObject && !state.selectedIndices.empty()) {
    OBJECTS::HandleObjectDrag(project, state, mouseWorld, camera);
    potentialChange = true;
  }
}

/*!***************************************************
 * @brief    Handles all Input
 * @details  Main entry point for handling input
 * @param    project Project&
 * @param    state InteractionState&
 * @param    camera Camera2D&
 * @return   void
 * @note
 * @date     2026.01.19
 * @author   bearded.griffin
 ****************************************************/
void HandleInput(Project &project, InteractionState &state, Camera2D &camera) {
  Vector2 mouseWorld = GetScreenToWorld2D(GetMousePosition(), camera);
  bool mouseHandledByUI = ImGui::GetIO().WantCaptureMouse;

  if (mouseHandledByUI)
    return;

  // Delete objects
  if (!state.selectedIndices.empty() &&
      (IsKeyPressed(KEY_DELETE) || IsKeyPressed(KEY_BACKSPACE))) {
    
    state.PushHistory(project);
    
    // Sort indices in descending order to delete without shifting issues
    std::vector<int> sortedIndices = state.selectedIndices;
    std::sort(sortedIndices.begin(), sortedIndices.end(), std::greater<int>());

    for (int idx : sortedIndices) {
      auto &objToDelete = project.objects[idx];
      if (objToDelete.texture.id != 0) UnloadTexture(objToDelete.texture);
      project.objects.erase(project.objects.begin() + idx);
    }

    project.isDirty = true;
    state.selectedIndices.clear();
    state.isDraggingObject = false;
    return;
  }

  // Panning
  if (IsKeyDown(KEY_SPACE) && IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
    Vector2 delta = GetMouseDeltaWorld(camera);
    camera.target = Vector2Add(camera.target, delta);
    return;
  }

  // Handle mouse interactions
  HandleMouseInteractions(project, state, mouseWorld, camera);

  // Zooming
  float wheel = GetMouseWheelMove();
  if (wheel != 0) {
    camera.zoom += wheel * ZOOM_SPEED;
    camera.zoom = std::max(camera.zoom, MIN_ZOOM);
  }

  // --- Arrow Key Nudging ---
  if (!state.selectedIndices.empty()) {
    static Project preNudgeState;
    static bool nudgeActive = false;

    bool up = IsKeyPressed(KEY_UP);
    bool down = IsKeyPressed(KEY_DOWN);
    bool left = IsKeyPressed(KEY_LEFT);
    bool right = IsKeyPressed(KEY_RIGHT);

    if (up || down || left || right) {
      if (!nudgeActive) {
        preNudgeState = project;
        nudgeActive = true;
      }

      float amount =
          (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) ? 10.0f : 1.0f;
      float dx = (right ? amount : 0) - (left ? amount : 0);
      float dy = (down ? amount : 0) - (up ? amount : 0);

      LabelSize canvasSz = LabelSizes[project.selectedLabelIndex];

      // Boundary check for group nudge
      float minX = 1e9, minY = 1e9, maxX = -1e9, maxY = -1e9;
      for (int idx : state.selectedIndices) {
        Rectangle b = OBJECTS::GetObjectBounds(project.objects[idx]);
        minX = std::min(minX, b.x);
        minY = std::min(minY, b.y);
        maxX = std::max(maxX, b.x + b.width);
        maxY = std::max(maxY, b.y + b.height);
      }

      if (minX + dx < 0) dx = -minX;
      if (minY + dy < 0) dy = -minY;
      if (maxX + dx > canvasSz.width) dx = canvasSz.width - maxX;
      if (maxY + dy > canvasSz.height) dy = canvasSz.height - maxY;

      if (dx != 0 || dy != 0) {
        for (int idx : state.selectedIndices) {
          project.objects[idx].x += dx;
          project.objects[idx].y += dy;
        }
        project.isDirty = true;
      }
    }

    // Push history once all arrow keys are released
    if (nudgeActive && !IsKeyDown(KEY_UP) && !IsKeyDown(KEY_DOWN) &&
        !IsKeyDown(KEY_LEFT) && !IsKeyDown(KEY_RIGHT)) {
      state.PushHistory(preNudgeState);
      nudgeActive = false;
    }
  }

  // --- Keyboard Shortcuts ---
  bool ctrl = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);

  // Undo (Ctrl+Z)
  if (ctrl && IsKeyPressed(KEY_Z) && !IsKeyDown(KEY_LEFT_SHIFT)) {
    state.Undo(project);
    state.selectedIndices.clear();
  }

  // Redo (Ctrl+Y or Ctrl+Shift+Z)
  if ((ctrl && IsKeyPressed(KEY_Y)) || (ctrl && IsKeyDown(KEY_LEFT_SHIFT) && IsKeyPressed(KEY_Z))) {
    state.Redo(project);
    state.selectedIndices.clear();
  }

  // Copy (Ctrl+C)
  if (ctrl && IsKeyPressed(KEY_C) && !state.selectedIndices.empty()) {
    state.clipboard.clear();
    for (int idx : state.selectedIndices) {
      state.clipboard.push_back(project.objects[idx]);
    }
  }

  // Cut (Ctrl+X)
  if (ctrl && IsKeyPressed(KEY_X) && !state.selectedIndices.empty()) {
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

  // Paste (Ctrl+V)
  if (ctrl && IsKeyPressed(KEY_V) && !state.clipboard.empty()) {
    state.PushHistory(project);
    state.selectedIndices.clear();
    for (const auto& obj : state.clipboard) {
      LabelObject newObj = obj;
      newObj.x += 10;
      newObj.y += 10;
      project.objects.push_back(newObj);
      state.selectedIndices.push_back((int)project.objects.size() - 1);
    }
    project.isDirty = true;
  }

  // --- New Keyboard Power User Shortcuts ---
  bool alt = IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT);

  // Add Objects (Alt + Key)
  if (alt && IsKeyPressed(KEY_T)) OBJECTS::AddTextObject(project, state);
  if (alt && IsKeyPressed(KEY_Q)) OBJECTS::AddQRCodeObject(project, state);
  if (alt && IsKeyPressed(KEY_B)) OBJECTS::AddBarcodeObject(project, state);
  if (alt && IsKeyPressed(KEY_L)) OBJECTS::AddLineObject(project, state);
  if (alt && IsKeyPressed(KEY_R)) OBJECTS::AddRectangleObject(project, state);
  if (alt && IsKeyPressed(KEY_C)) OBJECTS::AddCircleObject(project, state);
  if (alt && IsKeyPressed(KEY_D)) OBJECTS::AddBorderObject(project, state);
  if (alt && IsKeyPressed(KEY_F)) OBJECTS::AddFieldObject(project, state);

  // Rotation (Ctrl + [ / ])
  if (ctrl && (IsKeyPressed(KEY_LEFT_BRACKET) || IsKeyPressed(KEY_RIGHT_BRACKET))) {
    state.PushHistory(project);
    float step = IsKeyPressed(KEY_RIGHT_BRACKET) ? 15.0f : -15.0f;
    if (IsKeyDown(KEY_LEFT_SHIFT)) step *= 6.0f; // 90 degree jumps
    
    for (int idx : state.selectedIndices) {
        if (!project.objects[idx].isLocked) {
            project.objects[idx].rotation += step;
            // Normalize to 0-360
            while (project.objects[idx].rotation >= 360.0f) project.objects[idx].rotation -= 360.0f;
            while (project.objects[idx].rotation < 0.0f) project.objects[idx].rotation += 360.0f;
        }
    }
    project.isDirty = true;
  }

  // Printing (Ctrl + P for Single, Ctrl + Shift + P for Sequence)
  if (ctrl && IsKeyPressed(KEY_P)) {
      if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) {
          // Trigger Sequence Print - we can't easily trigger the popup from here 
          // without changing signatures, but we can do a default single increment print
          // or just let the user use the menu for the specific count.
      } else {
          Protocol::PrintLabel(project);
      }
  }

  // UI Toggles (Ctrl + G for Grid, Ctrl + H for Snapping)
  if (ctrl && IsKeyPressed(KEY_G)) Utils::appSettings.showGrid = !Utils::appSettings.showGrid;
  if (ctrl && IsKeyPressed(KEY_H)) {
      Utils::appSettings.snapToGrid = !Utils::appSettings.snapToGrid;
      Utils::appSettings.snapToObjects = Utils::appSettings.snapToGrid;
  }
}

/*!***************************************************
 * @brief    Creates the mouse delta
 * @details  Takes the mouse position to calculate
 * the mouse Delta.
 * @param    camera Camera2D
 * @return   Vector2Scale
 * @note
 * @date     2026.01.19
 * @author   bearded.griffin
 ****************************************************/
Vector2 GetMouseDeltaWorld(Camera2D camera) {
  Vector2 delta = GetMouseDelta();
  return Vector2Scale(delta, -1.0f / camera.zoom);
}

} // namespace INPUT_HANDLER