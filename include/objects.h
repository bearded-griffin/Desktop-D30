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
 * @file     include/objects.h
 * @brief    Handles all objects
 * @details  Handles object selection, resizing, and dragging
 * @note
 * @date     2026.02.03
 * @author   bearded.griffin
 ****************************************************/

#pragma once
#include "raylib.h"
#include "types.h"

// Object manipulation functions
namespace OBJECTS {
void HandleObjectSelection(Project &project, int &selectedIndex,
                           bool &isDraggingObject, Vector2 &dragOffset,
                           const Vector2 &mouseWorld, const Camera2D &camera);

void HandleObjectResize(Project &project, const int &selectedIndex,
                        ResizeHandle activeHandle, const Vector2 &mouseWorld,
                        const Camera2D &camera);

void HandleObjectDrag(Project &project, const int &selectedIndex,
                      const Vector2 &mouseWorld, const Vector2 &dragOffset,
                      const Camera2D &camera);

// Object creation functions
LabelObject CreateTextObject(const float &x, const float &y,
                             const std::string &text, const float &fontSize);

LabelObject CreateFieldObject(const float &x, const float &y,
                              const std::string &text, const float &fontSize);
LabelObject CreateRectangleObject(const float &x, const float &y,
                                  const float &width, const float &height,
                                  const float &cornerRadius = 0.0f);

LabelObject CreateCircleObject(const float &x, const float &y,
                               const float &radius);
LabelObject CreateBorderObject(const float &x, const float &y,
                               const LabelSize &sz, const int &radius = 10);

LabelObject CreateLineObject(const float &x, const float &y, const float &width,
                             const float &height, const float &thickness);

LabelObject CreateQRCodeObject(const float &x, const float &y,
                               const float &size, const std::string &data);

LabelObject CreateBarcodeObject(const float &x, const float &y,
                                const float &width, const float &height,
                                const std::string &data);

LabelObject CreateImageObject(const float &x, const float &y,
                              const float &width, const float &height,
                              const std::string &filePath);

// Object utility functions
bool IsObjectSelected(const Project &project, int index);
Rectangle GetObjectBounds(const LabelObject &obj);
void ClampObjectPosition(LabelObject &obj, const LabelSize &canvasSize);
void ValidateObjectSize(LabelObject &obj);
void UnloadProjectObjects(Project &project);
} // namespace OBJECTS