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
 * @file     include/input.h
 * @brief    Handles all Input
 * @details  Handles the mouse and keyboard interactions
 * with designing a label
 * @note
 * @date     2026.02.03
 ****************************************************/

#pragma once
#include "raylib.h"
#include "types.h"
#include "win_fix.h"

namespace UI {
struct UIState;
}

namespace INPUT_HANDLER {
void HandleMouseInteractions(Project &project, InteractionState &state,
                             const Vector2 &mouseWorld, const Camera2D &camera);
void HandleInput(Project &project, InteractionState &state, Camera2D &camera,
                 UI::UIState &uiState);
Vector2 GetMouseDeltaWorld(Camera2D camera);

} // namespace INPUT_HANDLER