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
#include "imgui.h"
#include "objects.h"
#include "utils.h"
#include <algorithm>

namespace INPUT {

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
  if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
    state.isResizing = false;
    state.activeHandle = HANDLE_NONE;

    if (state.selectedIndex != -1) {
      const auto &obj = project.objects[state.selectedIndex];
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
        Rectangle bounds = OBJECTS::GetObjectBounds(obj);
        float handleRadius = HANDLE_RADIUS / camera.zoom;
        Vector2 handlePositions[] = {
            {bounds.x, bounds.y},                               // Top-Left
            {bounds.x + bounds.width, bounds.y},                // Top-Right
            {bounds.x, bounds.y + bounds.height},               // Bottom-Left
            {bounds.x + bounds.width, bounds.y + bounds.height} // Bottom-Right
        };

        for (int i = 0; i < 4; i++) {
          if (CheckCollisionPointCircle(mouseWorld, handlePositions[i], handleRadius)) {
            if (i == 0) { // Top-Left: Delete
              auto &objToDelete = project.objects[state.selectedIndex];
              if (objToDelete.texture.id != 0) UnloadTexture(objToDelete.texture);
              project.objects.erase(project.objects.begin() + state.selectedIndex);
              project.isDirty = true;
              state.selectedIndex = -1;
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
      OBJECTS::HandleObjectSelection(project, state.selectedIndex,
                                     state.isDraggingObject, state.dragOffset,
                                     mouseWorld, camera);
    }
  }

  if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
    state.isDraggingObject = false;
    state.isResizing = false;
    state.activeHandle = HANDLE_NONE;
    SetMouseCursor(MOUSE_CURSOR_DEFAULT);
  }

  if (state.isResizing && state.selectedIndex != -1) {
    OBJECTS::HandleObjectResize(project, state.selectedIndex,
                                state.activeHandle, mouseWorld, camera);
  } else if (state.isDraggingObject && state.selectedIndex != -1) {
    OBJECTS::HandleObjectDrag(project, state.selectedIndex, mouseWorld,
                              state.dragOffset, camera);
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

  // Delete object
  if (state.selectedIndex != -1 &&
      (IsKeyPressed(KEY_DELETE) || IsKeyPressed(KEY_BACKSPACE))) {
    auto &objToDelete = project.objects[state.selectedIndex];
    if (objToDelete.texture.id != 0) UnloadTexture(objToDelete.texture);
    project.objects.erase(project.objects.begin() + state.selectedIndex);
    project.isDirty = true;
    state.selectedIndex = -1;
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

} // namespace INPUT