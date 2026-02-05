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
 * @file     include/rendering.h
 * @brief    Handles all rendering functionality
 * @details  Has an Object factory to draw the different
 * objects and the ability to render the entire canvas
 * to the format that is needed to print the label.
 * @note
 * @date     2026.02.03
 * @author   bearded.griffin
 ****************************************************/

#pragma once
#include "raylib.h"
#include "types.h"

namespace UI {
struct UIState;
}

namespace RENDERING {
void RenderTextObject(const LabelObject &obj, const Color &col,
                      const bool isSelected);
void RenderQRCode(const LabelObject &obj, const Color &col);
void RenderImageObject(LabelObject &obj);
void RenderLineObject(const LabelObject &obj, const Color &col);
void RenderShapeRect(const LabelObject &obj, const Color &col);
void RenderShapeCircle(const LabelObject &obj, const Color &col);
void RenderBarcode(const LabelObject &obj, const Color &col);
void RenderObject(LabelObject &obj, const bool isSelected,
                  const Camera2D &camera);
void DrawSelectionHandles(const LabelObject &obj, const Camera2D &camera);
void ImageDrawRoundedRectFilled(Image *dst, float x, float y, float w, float h,
                                float radius, Color col);
void DrawQRCode(const std::string &text, float x, float y, float size,
                Color color);
float DrawTextBox(Image *target, Font font, const char *text, float x, float y,
                  float fontSize, float spacing, Color tint, float maxWidth);
void DrawGrid(const LabelSize &currentSize);
Image RenderProjectToImage(const Project &project);
void RenderScene(Project &currentProject,
                 const InteractionState &interactionState,
                 const Camera2D &camera, const int &selectedIndex);

} // namespace RENDERING