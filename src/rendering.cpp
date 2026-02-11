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
 * @file     src/rendering.cpp
 * @brief    Handles all rendering functionality
 * @details  Has an Object factory to draw the different
 * objects and the ability to render the entire canvas
 * to the format that is needed to print the label.
 * @note
 * @date     2026.02.03
 * @author   bearded.griffin
 ****************************************************/

#include "rendering.h"
#include "assets.h"
#include "barcode.h"
#include "objects.h"
#include "qrcodegen.hpp"
#include "utils.h"
#include <algorithm>

using namespace qrcodegen;


namespace RENDERING {

/*!***************************************************
 * @brief    Renders the Text object
 * @details
 * @param    obj const LabelObject&
 * @param    col const Color&
 * @param    isSelected const bool
 * @return   void
 * @note
 * @date     2026.02.03
 * @author   bearded.griffin
 ****************************************************/
void RenderTextObject(const LabelObject &obj, const Color &col,
                      const bool isSelected) {
  Font displayFont = AssetManager::Get().GetFont(obj.fontName);
  DrawTextBox(nullptr, displayFont, obj.data.c_str(), obj.x, obj.y,
              obj.fontSize, 2.0f, col, obj.width);

  if (isSelected && obj.width > 0) {
    DrawRectangleLines(obj.x, obj.y, obj.width,
                       obj.height > 0 ? obj.height : obj.fontSize * 2,
                       Fade(SKYBLUE, 0.5f));
  }
}

/*!***************************************************
 * @brief    Renders the QR Code object
 * @details
 * @param    obj const LabelObject&
 * @param    col const Color&
 * @return   void
 * @note
 * @date     2026.02.03
 * @author   bearded.griffin
 ****************************************************/
void RenderQRCode(const LabelObject &obj, const Color &col) {
  DrawQRCode(obj.data, obj.x, obj.y, obj.width, col);
  DrawRectangleLines(obj.x, obj.y, obj.width, obj.width, Fade(GRAY, 0.3f));
}

/*!***************************************************
 * @brief    Renders the Image object
 * @details
 * @param    obj LabelObject&
 * @return   void
 * @note
 * @date     2026.02.03
 * @author   bearded.griffin
 ****************************************************/
void RenderImageObject(LabelObject &obj) {
  if (obj.texture.id == 0 && !obj.data.empty() &&
      FileExists(obj.data.c_str())) {
    Image img = LoadImage(obj.data.c_str());
    obj.texture = LoadTextureFromImage(img);
    UnloadImage(img);

    if (obj.width == 0)
      obj.width = (float)obj.texture.width;
    if (obj.height == 0)
      obj.height = (float)obj.texture.height;
  }

  if (obj.texture.id != 0) {
    Rectangle src = {0, 0, (float)obj.texture.width, (float)obj.texture.height};
    Rectangle dst = {obj.x, obj.y, obj.width, obj.height};
    DrawTexturePro(obj.texture, src, dst, {0, 0}, 0.0f, WHITE);
  } else {
    DrawRectangleLines(obj.x, obj.y, obj.width, obj.height, BLACK);
    DrawText("IMG", obj.x + 5, obj.y + 5, 10, BLACK);
  }
}

/*!***************************************************
 * @brief    Renders the Line object
 * @details
 * @param    obj const LabelObject&
 * @param    col const &Color
 * @return   void
 * @note
 * @date     2026.02.03
 * @author   bearded.griffin
 ****************************************************/
void RenderLineObject(const LabelObject &obj, const Color &col) {
  Vector2 start = {obj.x, obj.y};
  Vector2 end = {obj.x + obj.width, obj.y + obj.height};
  DrawLineEx(start, end, obj.fontSize, col);
}

/*!***************************************************
 * @brief    Renders the Rectangle shape
 * @details
 * @param    obj const LabelObject&
 * @param    col const &Color
 * @return   void
 * @note
 * @date     2026.02.03
 * @author   bearded.griffin
 ****************************************************/
void RenderShapeRect(const LabelObject &obj, const Color &col) {
  Rectangle rec = {obj.x, obj.y, obj.width, obj.height};
  float minDim = std::min(rec.width, rec.height);
  float roundness = (minDim > 0) ? (obj.cornerRadius / (minDim / 2.0f)) : 0.0f;
  roundness = std::clamp(roundness, 0.0f, 1.0f);

  DrawRectangleRounded(rec, roundness, 10, col);

  if (obj.type == ObjectType::Border) {
    float thick = obj.fontSize;
    if (thick * 2 < rec.width && thick * 2 < rec.height) {
      Rectangle inner = {rec.x + thick, rec.y + thick, rec.width - (thick * 2),
                         rec.height - (thick * 2)};
      float innerRadius = std::max(0.0f, obj.cornerRadius - thick);
      float innerMin = std::min(inner.width, inner.height);
      float innerRoundness =
          (innerMin > 0) ? (innerRadius / (innerMin / 2.0f)) : 0.0f;
      DrawRectangleRounded(inner, innerRoundness, 10, WHITE);
    }
  }
}

/*!***************************************************
 * @brief    Renders the Circle shape
 * @details
 * @param    obj const LabelObject&
 * @param    col const &Color
 * @return   void
 * @note
 * @date     2026.02.03
 * @author   bearded.griffin
 ****************************************************/
void RenderShapeCircle(const LabelObject &obj, const Color &col) {
  float radius = obj.width / 2.0f;
  Vector2 center = {obj.x + radius, obj.y + radius};
  DrawRing(center, radius - obj.fontSize, radius, 0, 360, 0, col);
}

/*!***************************************************
 * @brief    Renders the Barcode
 * @details
 * @param    obj const LabelObject&
 * @param    col const &Color
 * @return   void
 * @note
 * @date     2026.02.03
 * @author   bearded.griffin
 ****************************************************/
void RenderBarcode(const LabelObject &obj, const Color &col) {
  std::string code = Barcode::Encode128(obj.data);
  float moduleWidth = obj.width / (float)code.length();

  for (int i = 0; i < code.length(); i++) {
    if (code[i] == '1') {
      DrawRectangle((int)(obj.x + (i * moduleWidth)), (int)obj.y,
                    (int)(moduleWidth + BARCODE_MODULE_EXTRA), (int)obj.height,
                    col);
    }
  }
  DrawRectangleLines(obj.x, obj.y, obj.width, obj.height, Fade(GRAY, 0.5f));
}

/*!***************************************************
 * @brief    Renders specific objects
 * @details  Looks at the individual objects and renders
 * them.
 * @param    obj LabelObject&
 * @param    isSelected const bool
 * @param    camera const Camera2D&
 * @return   void
 * @note
 * @date     2026.02.03
 * @author   bearded.griffin
 ****************************************************/
void RenderObject(LabelObject &obj, const bool isSelected,
                  const Camera2D &camera) {
  Color col = GetColor(obj.colorHex);

  switch (obj.type) {
  case ObjectType::Text:
  case ObjectType::Field:
    RenderTextObject(obj, col, isSelected);
    break;
  case ObjectType::QRCode:
    RenderQRCode(obj, col);
    break;
  case ObjectType::Image:
    RenderImageObject(obj);
    break;
  case ObjectType::Line:
    RenderLineObject(obj, col);
    break;
  case ObjectType::ShapeRect:
  case ObjectType::Border:
    RenderShapeRect(obj, col);
    break;
  case ObjectType::ShapeCircle:
    RenderShapeCircle(obj, col);
    break;
  case ObjectType::Barcode:
    RenderBarcode(obj, col);
    break;
  }

  if (isSelected) {
    DrawSelectionHandles(obj, camera);
  }
}

/*!***************************************************
 * @brief    Draws selection handles
 * @details
 * @param    obj const LabelObject&
 * @param    camera const Camera2D&
 * @return   void
 * @note
 * @date     2026.02.01
 * @author   bearded.griffin
 ****************************************************/
void DrawSelectionHandles(const LabelObject &obj, const Camera2D &camera) {
  if (obj.type == ObjectType::Line) {
    float handleRadius = HANDLE_RADIUS / camera.zoom;
    Vector2 start = {obj.x, obj.y};
    Vector2 end = {obj.x + obj.width, obj.y + obj.height};
    DrawLineEx(start, end, 1.0f / camera.zoom, SKYBLUE);
    DrawCircleV(start, handleRadius, SKYBLUE);
    DrawCircleV(end, handleRadius, SKYBLUE);
  } else {
    Rectangle bounds = OBJECTS::GetObjectBounds(obj);
    DrawRectangleLinesEx(bounds, 1.0f / camera.zoom, SKYBLUE);

    float handleSize = HANDLE_SIZE / camera.zoom;
    Rectangle handles[] = {
        {bounds.x - handleSize / 2, bounds.y - handleSize / 2, handleSize,
         handleSize},
        {bounds.x + bounds.width - handleSize / 2, bounds.y - handleSize / 2,
         handleSize, handleSize},
        {bounds.x - handleSize / 2, bounds.y + bounds.height - handleSize / 2,
         handleSize, handleSize},
        {bounds.x + bounds.width - handleSize / 2,
         bounds.y + bounds.height - handleSize / 2, handleSize, handleSize}};

    for (const auto &handle : handles) {
      DrawRectangleRec(handle, SKYBLUE);
    }
  }
}

/*!***************************************************
 * @brief    Draw the QR Code Object
 * @details  Takes the text passed to it and draws
 * the QR Code object on the canvas.
 * @param    text const std::string&
 * @param    x float
 * @param    y float
 * @param    size float
 * @param    color Color
 * @return   void
 * @note
 * @date     2026.01.19
 * @author   bearded.griffin
 ****************************************************/
void DrawQRCode(const std::string &text, float x, float y, float size,
                Color color) {
  if (text.empty())
    return;

  // 1. Generate the QR Data
  // Ecc::MEDIUM allows for ~15% error correction (good for printing)
  QrCode qr = QrCode::encodeText(text.c_str(), QrCode::Ecc::MEDIUM);

  // 2. Calculate drawing metrics
  int gridSize = qr.getSize(); // e.g., 21, 25, etc.
  if (gridSize <= 0)
    return;

  // Size of one little square (module)
  float moduleSize = size / (float)gridSize;

  // 3. Draw the modules
  // QR codes are usually black on white, but we'll use the user's color
  for (int yModule = 0; yModule < gridSize; yModule++) {
    for (int xModule = 0; xModule < gridSize; xModule++) {
      if (qr.getModule(xModule, yModule)) {
        DrawRectangle(
            (int)(x + (xModule * moduleSize)),
            (int)(y + (yModule * moduleSize)),
            (int)(moduleSize +
                  1), // +1 to fix tiny gaps between floating point rects
            (int)(moduleSize + 1), color);
      }
    }
  }
}

/*!***************************************************
 * @brief    Renders the canvas for printing
 * @details  Takes the entire project and converts it
 * to a black and white image so that it can be sent
 * to the printer.
 * @param    project const Project&
 * @return   Image
 * @note
 * @date     2026.01.20
 * @author   bearded.griffin
 ****************************************************/
Image RenderProjectToImage(const Project &project) {
  // 1. Get Canvas Dimensions
  LabelSize currentSize = LabelSizes[project.selectedLabelIndex];
  int width = (int)currentSize.width;
  int height = (int)currentSize.height;

  // 2. Create a Blank White Image
  Image canvas = GenImageColor(width, height, WHITE);

  // 3. Draw Objects onto the Image
  for (const auto &obj : project.objects) {
    if (obj.type == ObjectType::Text || obj.type == ObjectType::Field) {
      Font printFont = AssetManager::Get().GetFont(obj.fontName);
      DrawTextBox(&canvas, printFont, obj.data.c_str(), obj.x, obj.y,
                  obj.fontSize, 2.0f, BLACK, obj.width);
    } else if (obj.type == ObjectType::QRCode) {
      if (obj.data.empty())
        continue;

      // Manual QR Drawing for Image Buffer
      QrCode qr = QrCode::encodeText(obj.data.c_str(), QrCode::Ecc::MEDIUM);
      int gridSize = qr.getSize();
      if (gridSize > 0) {
        float moduleSize = obj.width / (float)gridSize;

        for (int yModule = 0; yModule < gridSize; yModule++) {
          for (int xModule = 0; xModule < gridSize; xModule++) {
            if (qr.getModule(xModule, yModule)) {
              int px = (int)(obj.x + (xModule * moduleSize));
              int py = (int)(obj.y + (yModule * moduleSize));
              int pSize = (int)(moduleSize + 1);

              // Use ImageDrawRectangle (CPU) not DrawRectangle (GPU)
              ImageDrawRectangle(&canvas, px, py, pSize, pSize, BLACK);
            }
          }
        }
      }
    } else if (obj.type == ObjectType::Image) {
      if (FileExists(obj.data.c_str())) {
        Image srcImg = LoadImage(obj.data.c_str());
        // Ensure we are in a format we can manipulate easily
        ImageFormat(&srcImg, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);

        // --- CONVERT TO B&W ---
        // Manual Grayscale and Thresholding with Alpha support
        Color *pixels = (Color *)srcImg.data;
        for (int i = 0; i < srcImg.width * srcImg.height; i++) {
          // If the pixel is transparent, treat it as the white label background
          if (pixels[i].a < 128) {
            pixels[i] = WHITE;
          } else {
            // Grayscale conversion (weighted for better perception)
            unsigned char gray = (unsigned char)(0.299f * pixels[i].r +
                                                 0.587f * pixels[i].g +
                                                 0.114f * pixels[i].b);

            // Apply thresholding
            if (gray < 128) {
              pixels[i] = BLACK;
            } else {
              pixels[i] = WHITE;
            }
          }
        }

        // Resize to target dimensions
        ImageResize(&srcImg, (int)obj.width, (int)obj.height);

        // Draw onto canvas
        Rectangle srcRec = {0, 0, (float)srcImg.width, (float)srcImg.height};
        Rectangle dstRec = {obj.x, obj.y, (float)srcImg.width,
                            (float)srcImg.height};

        ImageDraw(&canvas, srcImg, srcRec, dstRec, WHITE);

        UnloadImage(srcImg);
      } else {
        // Fallback if file missing
        ImageDrawRectangleLines(&canvas, {obj.x, obj.y, obj.width, obj.height},
                                2, BLACK);
        ImageDrawText(&canvas, "FILE NOT FOUND", (int)obj.x + 5, (int)obj.y + 5,
                      10, BLACK);
      }
      // Render Basic Shapes
    } else if (obj.type == ObjectType::Line) {
      // Draw a line from (x,y) to (x+width, y+height)
      // We use 'fontSize' as the Line Thickness
      Vector2 start = {obj.x, obj.y};
      Vector2 end = {obj.x + obj.width, obj.y + obj.height};
      ImageDrawLineEx(&canvas, start, end, (int)obj.fontSize, BLACK);
    } else if (obj.type == ObjectType::ShapeRect ||
               obj.type == ObjectType::Border) {
      // 1. Draw the Outer Box (Black)
      ImageDrawRoundedRectFilled(&canvas, obj.x, obj.y, obj.width, obj.height,
                                 obj.cornerRadius, BLACK);

      // 2. Calculate thickness
      float thick = obj.fontSize;

      // 3. Draw the Inner Box (White) to create the "Hollow" effect
      if (thick * 2 < obj.width && thick * 2 < obj.height) {
        ImageDrawRoundedRectFilled(
            &canvas, obj.x + thick, obj.y + thick, obj.width - (thick * 2),
            obj.height - (thick * 2),
            obj.cornerRadius > thick ? obj.cornerRadius - thick : 0, WHITE);
      }
    } else if (obj.type == ObjectType::ShapeCircle) {
      // Raylib doesn't have ImageDrawCircleLines with thickness easily.
      // We can simulate it or just use a filled circle for now,
      // but let's stick to Rectangle and Line for v1 as they are most useful
      // for labels. If you really want circles, we can use ImageDrawCircle but
      // it's filled.

      // Workaround: Draw Circle (filled black) then smaller Circle (filled
      // white)
      int radius = (int)(obj.width / 2.0f);
      int centerX = (int)(obj.x + radius);
      int centerY = (int)(obj.y + radius);
      ImageDrawCircle(&canvas, centerX, centerY, radius, BLACK);
      ImageDrawCircle(&canvas, centerX, centerY, radius - (int)obj.fontSize,
                      WHITE);
    } else if (obj.type == ObjectType::Barcode) {
      std::string code = Barcode::Encode128(obj.data);

      // Calculate bar width based on object width
      // Total modules = code.length()
      // We scale the bars to fit the user's box
      float moduleWidth = obj.width / (float)code.length();

      for (int i = 0; i < code.length(); i++) {
        if (code[i] == '1') {
          Rectangle bar = {obj.x + (i * moduleWidth), obj.y,
                           moduleWidth +
                               0.5f, // +0.5 to fix floating point gaps
                           obj.height};
          ImageDrawRectangle(&canvas, (int)bar.x, (int)bar.y, (int)bar.width,
                             (int)bar.height, BLACK);
        }
      }

      // Optional: Draw text below it?
      // Usually we just draw the bars, user can add a Text object below if they
      // want.
    }
  }
  return canvas;
}

/*!***************************************************
 * @brief    Draws a basic border
 * @details  Draws a basic border that can have the
 * corners rounded.
 * @param    dst Image*
 * @param    x float
 * @param    y float
 * @param    w float
 * @param    h float
 * @param    radius float
 * @param    col Color
 * @return   void
 * @note
 * @date     2026.01.26
 * @author   bearded.griffin
 ****************************************************/
void ImageDrawRoundedRectFilled(Image *dst, float x, float y, float w, float h,
                                float radius, Color col) {
  if (radius <= 0) {
    ImageDrawRectangle(dst, (int)x, (int)y, (int)w, (int)h, col);
    return;
  }

  // 1. Draw 4 Corner Circles
  int r = (int)radius;
  ImageDrawCircle(dst, (int)(x + r), (int)(y + r), r, col);     // Top-Left
  ImageDrawCircle(dst, (int)(x + w - r), (int)(y + r), r, col); // Top-Right
  ImageDrawCircle(dst, (int)(x + r), (int)(y + h - r), r, col); // Bottom-Left
  ImageDrawCircle(dst, (int)(x + w - r), (int)(y + h - r), r,
                  col); // Bottom-Right

  // 2. Draw 2 Crossing Rectangles
  // Horizontal bar (covers left to right, inset by radius height)
  ImageDrawRectangle(dst, (int)x, (int)(y + r), (int)w, (int)(h - 2 * r), col);
  // Vertical bar (covers top to bottom, inset by radius width)
  ImageDrawRectangle(dst, (int)(x + r), (int)y, (int)(w - 2 * r), (int)h, col);
}

/*!***************************************************
 * @brief    Handles word wrapping
 * @details  Draws text inside a specific width. If width <= 0, draws normally
 * (one line). Returns the total height used (so we can auto-size the box if
 * needed).
 * @param    target Image*
 * @param    font Font
 * @param    text const char*
 * @param    x float
 * @param    y float
 * @param    fontsize float
 * @param    spacing float
 * @param    tint Color
 * @param    maxWidth float
 * @return   float
 * @note
 * @date     2026.01.24
 * @author   bearded.griffin
 ****************************************************/
float DrawTextBox(Image *target, Font font, const char *text, float x, float y,
                  float fontSize, float spacing, Color tint, float maxWidth) {
  if (maxWidth <= 0) {
    // Legacy Mode: No wrapping
    if (target)
      ImageDrawTextEx(target, font, text, {x, y}, fontSize, spacing, tint);
    else
      DrawTextEx(font, text, {x, y}, fontSize, spacing,
                 tint); // <--- Screen Draw
    return fontSize;
  }

  // Wrapping Logic
  std::string textStr = text;
  std::vector<std::string> words;
  std::string currentWord;

  // 1. Split into words
  for (char c : textStr) {
    if (c == ' ' || c == '\n') {
      if (!currentWord.empty())
        words.push_back(currentWord);
      if (c == '\n')
        words.push_back("\n");
      currentWord = "";
    } else {
      currentWord += c;
    }
  }
  if (!currentWord.empty())
    words.push_back(currentWord);

  // 2. Build Lines
  float currentX = 0;
  float currentY = 0;
  float spaceWidth = MeasureTextEx(font, " ", fontSize, spacing).x;

  for (const auto &word : words) {
    if (word == "\n") {
      currentY += fontSize;
      currentX = 0;
      continue;
    }

    Vector2 wordSize = MeasureTextEx(font, word.c_str(), fontSize, spacing);

    // Check if word fits
    if (currentX + wordSize.x > maxWidth && currentX > 0) {
      // New Line
      currentY += fontSize;
      currentX = 0;
    }

    // Draw
    if (target) {
      // Draw to Image (Printer)
      ImageDrawTextEx(target, font, word.c_str(), {x + currentX, y + currentY},
                      fontSize, spacing, tint);
    } else {
      // Draw to Screen (Editor)
      DrawTextEx(font, word.c_str(), {x + currentX, y + currentY}, fontSize,
                 spacing, tint);
    }

    currentX += wordSize.x + spaceWidth;
  }

  return currentY + fontSize;
}

/*!***************************************************
 * @brief    Renders the main scene
 * @details  Renders the entire label canvas with
 * all objects.
 * @param    currentProject Project&
 * @param    interactionState const InteractionState&
 * @param    camera const Camera2D&
 * @param    selectedIndex const int&
 * @return   void
 * @note
 * @date     2026.02.01
 * @author   bearded.griffin
 ****************************************************/
void RenderScene(Project &currentProject,
                 const InteractionState &interactionState,
                 const Camera2D &camera, const int &selectedIndex) {

  BeginDrawing();
  ClearBackground(Utils::appSettings.darkTheme ? Color{40, 40, 40, 255} : RAYWHITE);

  BeginMode2D(camera);

  LabelSize currentSize = LabelSizes[currentProject.selectedLabelIndex];
  DrawRectangle(0, 0, (int)currentSize.width, (int)currentSize.height, WHITE);
  DrawRectangleLines(0, 0, (int)currentSize.width, (int)currentSize.height,
                     GRAY);

  if (Utils::appSettings.showGrid) {
    DrawGrid(currentSize);
  }

  // Object rendering
  for (int i = 0; i < currentProject.objects.size(); i++) {
    auto &obj = currentProject.objects[i];
    Color col = GetColor(obj.colorHex);

    if (obj.type == ObjectType::Text || obj.type == ObjectType::Field) {
      Font displayFont = AssetManager::Get().GetFont(obj.fontName);
      DrawTextBox(nullptr, displayFont, obj.data.c_str(), obj.x, obj.y,
                  obj.fontSize, 2.0f, col, obj.width);

      if (i == selectedIndex && obj.width > 0) {
        DrawRectangleLines(obj.x, obj.y, obj.width,
                           obj.height > 0 ? obj.height : obj.fontSize * 2,
                           Fade(SKYBLUE, 0.5f));
      }
    } else if (obj.type == ObjectType::QRCode) {
      DrawQRCode(obj.data, obj.x, obj.y, obj.width, col);
      DrawRectangleLines(obj.x, obj.y, obj.width, obj.width, Fade(GRAY, 0.3f));
    } else if (obj.type == ObjectType::Image) {
      if (obj.texture.id == 0 && !obj.data.empty() &&
          FileExists(obj.data.c_str())) {
        Image img = LoadImage(obj.data.c_str());
        obj.texture = LoadTextureFromImage(img);
        UnloadImage(img);

        if (obj.width == 0)
          obj.width = (float)obj.texture.width;
        if (obj.height == 0)
          obj.height = (float)obj.texture.height;
      }

      if (obj.texture.id != 0) {
        Rectangle src = {0, 0, (float)obj.texture.width,
                         (float)obj.texture.height};
        Rectangle dst = {obj.x, obj.y, obj.width, obj.height};
        DrawTexturePro(obj.texture, src, dst, {0, 0}, 0.0f, WHITE);
      } else {
        DrawRectangleLines(obj.x, obj.y, obj.width, obj.height, BLACK);
        DrawText("IMG", obj.x + 5, obj.y + 5, 10, BLACK);
      }
    } else if (obj.type == ObjectType::Line) {
      Vector2 start = {obj.x, obj.y};
      Vector2 end = {obj.x + obj.width, obj.y + obj.height};
      DrawLineEx(start, end, obj.fontSize, col);
    } else if (obj.type == ObjectType::ShapeRect ||
               obj.type == ObjectType::Border) {
      Rectangle rec = {obj.x, obj.y, obj.width, obj.height};
      float minDim = std::min(rec.width, rec.height);
      float roundness =
          (minDim > 0) ? (obj.cornerRadius / (minDim / 2.0f)) : 0.0f;
      roundness = std::clamp(roundness, 0.0f, 1.0f);

      DrawRectangleRounded(rec, roundness, 10, col);

      float thick = obj.fontSize;
      if (thick * 2 < rec.width && thick * 2 < rec.height) {
        Rectangle inner = {rec.x + thick, rec.y + thick,
                           rec.width - (thick * 2), rec.height - (thick * 2)};
        float innerRadius = std::max(0.0f, obj.cornerRadius - thick);
        float innerMin = std::min(inner.width, inner.height);
        float innerRoundness =
            (innerMin > 0) ? (innerRadius / (innerMin / 2.0f)) : 0.0f;
        DrawRectangleRounded(inner, innerRoundness, 10, WHITE);
      }
    } else if (obj.type == ObjectType::ShapeCircle) {
      float radius = obj.width / 2.0f;
      Vector2 center = {obj.x + radius, obj.y + radius};
      DrawRing(center, radius - obj.fontSize, radius, 0, 360, 0, col);
    } else if (obj.type == ObjectType::Barcode) {
      std::string code = Barcode::Encode128(obj.data);
      float moduleWidth = obj.width / (float)code.length();

      for (int i = 0; i < code.length(); i++) {
        if (code[i] == '1') {
          DrawRectangle((int)(obj.x + (i * moduleWidth)), (int)obj.y,
                        (int)(moduleWidth + BARCODE_MODULE_EXTRA),
                        (int)obj.height, col);
        }
      }
      DrawRectangleLines(obj.x, obj.y, obj.width, obj.height, Fade(GRAY, 0.5f));
    }

    if (i == selectedIndex) {
      DrawSelectionHandles(obj, camera);
    }
  }

  EndMode2D();
}

/*!***************************************************
 * @brief    Draws the grid
 * @details
 * @param    currentSize const LabelSize&
 * @return   void
 * @note
 * @date     2026.02.01
 * @author   bearded.griffin
 ****************************************************/
void DrawGrid(const LabelSize &currentSize) {
  for (int x = 0; x <= currentSize.width; x += GRID_SIZE) {
    DrawLine(x, 0, x, currentSize.height, LIGHTGRAY);
  }
  for (int y = 0; y <= currentSize.height; y += GRID_SIZE) {
    DrawLine(0, y, currentSize.width, y, LIGHTGRAY);
  }
}


} // namespace RENDERING