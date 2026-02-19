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
 * @details
 * @note
 * @date     2026.02.03
 * @author   bearded.griffin
 ****************************************************/

#include "rendering.h"
#include "assets.h"
#include "barcode.h"
#include "objects.h"
#include "raylib.h"
#include "types.h"
#include "utils.h"
#include <algorithm>
#include <cmath>
#include <qrcodegen.hpp>
#include <sstream>
#include <vector>

using qrcodegen::QrCode;

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
              obj.fontSize, 2.0f, col, obj.width, obj.rotation);

  if (isSelected && obj.width > 0) {
    // Note: Bounding box for selection remains axis-aligned for now, 
    // or we'd need DrawRectangleLinesPro
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
void RenderQRCode(LabelObject &obj, const Color &col) {
  if (obj.texture.id == 0 || obj.data != obj.lastData ||
      obj.colorHex != obj.lastColor) {
    if (obj.texture.id != 0)
      UnloadTexture(obj.texture);

    // Generate a high-quality QR image for the UI
    // We use a fixed size for the cache texture to keep it sharp
    int cacheSize = 256;

    Image qrImg = GenImageColor(cacheSize, cacheSize, BLANK);

    // Manual QR Drawing to the Image
    QrCode qr = QrCode::encodeText(obj.data.c_str(), QrCode::Ecc::MEDIUM);
    int gridSize = qr.getSize();
    if (gridSize > 0) {
      float moduleSize = (float)cacheSize / (float)gridSize;
      for (int yModule = 0; yModule < gridSize; yModule++) {
        for (int xModule = 0; xModule < gridSize; xModule++) {
          if (qr.getModule(xModule, yModule)) {
            ImageDrawRectangle(&qrImg, (int)(xModule * moduleSize),
                               (int)(yModule * moduleSize), (int)moduleSize + 1,
                               (int)moduleSize + 1, col);
          }
        }
      }
    }
    obj.texture = LoadTextureFromImage(qrImg);
    UnloadImage(qrImg);
    obj.lastData = obj.data;
    obj.lastColor = obj.colorHex;
  }
  DrawTexturePro(obj.texture,
                 {0, 0, (float)obj.texture.width, (float)obj.texture.height},
                 {obj.x, obj.y, obj.width, obj.width}, {0, 0}, obj.rotation, WHITE);
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
  if ((obj.texture.id == 0 || obj.lastThreshold != obj.threshold) && 
      !obj.data.empty() && FileExists(obj.data.c_str())) {
    
    if (obj.texture.id != 0) UnloadTexture(obj.texture);

    Image img = LoadImage(obj.data.c_str());
    ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);

    Color *pixels = (Color *)img.data;
    for (int i = 0; i < img.width * img.height; i++) {
        if (pixels[i].a < 128) {
            pixels[i] = WHITE; // Transparent becomes white
        } else {
            // Grayscale conversion
            unsigned char gray = (unsigned char)(0.299f * pixels[i].r + 0.587f * pixels[i].g + 0.114f * pixels[i].b);
            // Apply threshold
            if (gray < obj.threshold) {
                pixels[i] = BLACK;
            } else {
                pixels[i] = WHITE;
            }
        }
    }

    obj.texture = LoadTextureFromImage(img);
    UnloadImage(img);
    obj.lastThreshold = obj.threshold;

    if (obj.width == 0)
      obj.width = (float)obj.texture.width;
    if (obj.height == 0)
      obj.height = (float)obj.texture.height;
  }

  if (obj.texture.id != 0) {
    Rectangle src = {0, 0, (float)obj.texture.width, (float)obj.texture.height};
    Rectangle dst = {obj.x, obj.y, obj.width, obj.height};
    DrawTexturePro(obj.texture, src, dst, {0, 0}, obj.rotation, WHITE);
  } else {
    DrawRectangleLines(obj.x, obj.y, obj.width, obj.height, BLACK);
    Font f = AssetManager::Get().GetDefaultFont();
    DrawTextEx(f, "IMG", {obj.x + 5, obj.y + 5}, 10, 1.0f, BLACK);
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
  if (obj.rotation == 0) {
    DrawRectangleRounded({obj.x, obj.y, obj.width, obj.height},
                         obj.cornerRadius / (obj.width / 2.0f), 10, col);

    float thick = obj.fontSize;
    if (thick > 0) {
      DrawRectangleRounded(
          {obj.x + thick, obj.y + thick, obj.width - (thick * 2),
           obj.height - (thick * 2)},
          obj.cornerRadius / ((obj.width - thick * 2) / 2.0f), 10, WHITE);
    }
  } else {
    DrawRectanglePro({obj.x, obj.y, obj.width, obj.height}, {0, 0}, obj.rotation,
                     col);
    float thick = obj.fontSize;
    if (thick > 0) {
      Vector2 innerPos = Vector2Rotate({thick, thick}, obj.rotation * DEG2RAD);
      DrawRectanglePro(
          {obj.x + innerPos.x, obj.y + innerPos.y, obj.width - (thick * 2),
           obj.height - (thick * 2)},
          {0, 0}, obj.rotation, WHITE);
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
void RenderBarcode(LabelObject &obj, const Color &col) {
  if (obj.texture.id == 0 || obj.data != obj.lastData ||
      obj.colorHex != obj.lastColor) {
    if (obj.texture.id != 0)
      UnloadTexture(obj.texture);

    std::string code = Barcode::Encode128(obj.data);
    if (!code.empty()) {
      // Use a high-resolution cache for the barcode
      int cacheWidth = 512;
      int cacheHeight = 64;
      Image barImg = GenImageColor(cacheWidth, cacheHeight, BLANK);

      float moduleWidth = (float)cacheWidth / (float)code.length();
      for (int i = 0; i < code.length(); i++) {
        if (code[i] == '1') {
          ImageDrawRectangle(&barImg, (int)(i * moduleWidth), 0,
                             (int)moduleWidth + 1, cacheHeight, col);
        }
      }
      obj.texture = LoadTextureFromImage(barImg);
      UnloadImage(barImg);
    }
    obj.lastData = obj.data;
    obj.lastColor = obj.colorHex;
  }

  if (obj.texture.id != 0) {
    DrawTexturePro(obj.texture,
                   {0, 0, (float)obj.texture.width, (float)obj.texture.height},
                   {obj.x, obj.y, obj.width, obj.height}, {0, 0}, obj.rotation, WHITE);
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
  Color primaryCol = obj.isLocked ? GRAY : SKYBLUE;
  Color accentCol = obj.isLocked ? DARKGRAY : DARKBLUE;

  if (obj.type == ObjectType::Line) {
    float handleRadius = HANDLE_RADIUS / camera.zoom;
    Vector2 start = {obj.x, obj.y};
    Vector2 end = {obj.x + obj.width, obj.y + obj.height};
    DrawLineEx(start, end, 1.0f / camera.zoom, primaryCol);
    DrawCircleV(start, handleRadius, primaryCol);
    DrawCircleV(end, handleRadius, primaryCol);
  } else {
    Rectangle localBounds = {0, 0, obj.width, obj.height};
    if ((obj.type == ObjectType::Text || obj.type == ObjectType::Field)) {
        if (localBounds.height <= 0) localBounds.height = obj.fontSize * 1.5f;
        if (localBounds.width <= 0) {
            Font f = AssetManager::Get().GetFont(obj.fontName);
            localBounds.width = MeasureTextEx(f, obj.data.c_str(), obj.fontSize, 2.0f).x;
        }
    }

    Vector2 p1 = {0, 0};
    Vector2 p2 = {localBounds.width, 0};
    Vector2 p3 = {localBounds.width, localBounds.height};
    Vector2 p4 = {0, localBounds.height};

    if (obj.rotation != 0) {
        p1 = Vector2Rotate(p1, obj.rotation * DEG2RAD);
        p2 = Vector2Rotate(p2, obj.rotation * DEG2RAD);
        p3 = Vector2Rotate(p3, obj.rotation * DEG2RAD);
        p4 = Vector2Rotate(p4, obj.rotation * DEG2RAD);
    }

    p1 = Vector2Add(p1, {obj.x, obj.y});
    p2 = Vector2Add(p2, {obj.x, obj.y});
    p3 = Vector2Add(p3, {obj.x, obj.y});
    p4 = Vector2Add(p4, {obj.x, obj.y});

    DrawLineEx(p1, p2, 1.0f / camera.zoom, primaryCol);
    DrawLineEx(p2, p3, 1.0f / camera.zoom, primaryCol);
    DrawLineEx(p3, p4, 1.0f / camera.zoom, primaryCol);
    DrawLineEx(p4, p1, 1.0f / camera.zoom, primaryCol);

    float handleRadius = HANDLE_RADIUS / camera.zoom;
    Vector2 handles[] = {p1, p2, p4, p3}; // Top-Left, Top-Right, Bottom-Left, Bottom-Right

    for (int i = 0; i < 4; i++) {
      DrawCircleV(handles[i], handleRadius, primaryCol);
      DrawCircleLinesV(handles[i], handleRadius, accentCol);

      if (i == 0) { // Top-Left: Delete (X) or Lock Icon
        if (obj.isLocked) {
            // Draw a simple lock body
            float pad = handleRadius * 0.4f;
            DrawRectangleV({handles[i].x - pad, handles[i].y}, {pad * 2, pad}, WHITE);
            // Draw a lock shackle
            DrawCircleLinesV({handles[i].x, handles[i].y}, pad, WHITE);
        } else {
            float xSize = handleRadius * 0.6f;
            DrawLineEx({handles[i].x - xSize, handles[i].y - xSize},
                       {handles[i].x + xSize, handles[i].y + xSize},
                       2.0f / camera.zoom, WHITE);
            DrawLineEx({handles[i].x + xSize, handles[i].y - xSize},
                       {handles[i].x - xSize, handles[i].y + xSize},
                       2.0f / camera.zoom, WHITE);
        }
      } else if (i == 3) { // Bottom-Right: Resize (Arrows)
        if (obj.isLocked) continue;
        float aSize = handleRadius * 0.6f;
        // Simple dot for now if rotated to avoid complex arrow math
        if (obj.rotation == 0) {
            DrawLineEx({handles[i].x - aSize, handles[i].y - aSize},
                       {handles[i].x + aSize, handles[i].y + aSize},
                       2.0f / camera.zoom, WHITE);
            DrawLineEx({handles[i].x + aSize, handles[i].y + aSize},
                       {handles[i].x + aSize - aSize / 2, handles[i].y + aSize},
                       2.0f / camera.zoom, WHITE);
            DrawLineEx({handles[i].x + aSize, handles[i].y + aSize},
                       {handles[i].x + aSize, handles[i].y + aSize - aSize / 2},
                       2.0f / camera.zoom, WHITE);
        } else {
            DrawCircleV(handles[i], handleRadius * 0.4f, WHITE);
        }
      }
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

  float moduleSize = size / (float)gridSize;

  // 3. Draw the modules
  // QR codes are usually black on white, but we'll use the user's color
  for (int yModule = 0; yModule < gridSize; yModule++) {
    for (int xModule = 0; xModule < gridSize; xModule++) {
      if (qr.getModule(xModule, yModule)) {
        float fx = x + (xModule * moduleSize);
        float fy = y + (yModule * moduleSize);

        int xStart = (int)roundf(fx);
        int yStart = (int)roundf(fy);
        int xEnd = (int)roundf(fx + moduleSize);
        int yEnd = (int)roundf(fy + moduleSize);

        DrawRectangle(xStart, yStart, xEnd - xStart, yEnd - yStart, color);
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
    if (!obj.isVisible) continue;
    if (obj.type == ObjectType::Text || obj.type == ObjectType::Field) {
      Font printFont = AssetManager::Get().GetFont(obj.fontName);
      DrawTextBox(&canvas, printFont, obj.data.c_str(), obj.x, obj.y,
                  obj.fontSize, 2.0f, BLACK, obj.width, obj.rotation);
    } else if (obj.type == ObjectType::QRCode) {
      // Manual QR Drawing for Image Buffer
      QrCode qr = QrCode::encodeText(obj.data.c_str(), QrCode::Ecc::MEDIUM);
      int gridSize = qr.getSize();
      if (gridSize > 0) {
        float moduleSize = obj.width / (float)gridSize;

        for (int yModule = 0; yModule < gridSize; yModule++) {
          for (int xModule = 0; xModule < gridSize; xModule++) {
            if (qr.getModule(xModule, yModule)) {
              // Calculate module bounds in float for precision, then
              // floor/ceil for integer buffer
              float fx = obj.x + (xModule * moduleSize);
              float fy = obj.y + (yModule * moduleSize);

              // We want to avoid gaps, but also avoid over-bleeding.
              // Rounding to nearest integer for the start/end positions is
              // safer for a pixel buffer.
              int xStart = (int)roundf(fx);
              int yStart = (int)roundf(fy);
              int xEnd = (int)roundf(fx + moduleSize);
              int yEnd = (int)roundf(fy + moduleSize);

              ImageDrawRectangle(&canvas, xStart, yStart, xEnd - xStart,
                                 yEnd - yStart, BLACK);
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
          // If the pixel is transparent, treat it as the white label
          // background
          if (pixels[i].a < 128) {
            pixels[i] = WHITE;
          } else {
            // Grayscale conversion (weighted for better perception)
            unsigned char gray =
                (unsigned char)(0.299f * pixels[i].r + 0.587f * pixels[i].g +
                                0.114f * pixels[i].b);

            // Apply thresholding
            if (gray < obj.threshold) {
              pixels[i] = BLACK;
            } else {
              pixels[i] = WHITE;
            }
          }
        }

        // Resize to target dimensions with safety clamp
        int targetW = std::clamp((int)obj.width, 1, (int)MAX_OBJECT_SIZE);
        int targetH = std::clamp((int)obj.height, 1, (int)MAX_OBJECT_SIZE);
        ImageResize(&srcImg, targetW, targetH);

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
      // for labels. If you really want circles, we can use ImageDrawCircle
      // but it's filled.

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
          float fx = obj.x + (i * moduleWidth);
          int xStart = (int)roundf(fx);
          int xEnd = (int)roundf(fx + moduleWidth);

          ImageDrawRectangle(&canvas, xStart, (int)obj.y, xEnd - xStart,
                             (int)obj.height, BLACK);
        }
      }

      // Optional: Draw text below it?
      // Usually we just draw the bars, user can add a Text object below if
      // they want.
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

  // Draw the main rectangles (center, top/bottom, left/right)
  ImageDrawRectangle(dst, (int)(x + radius), (int)y, (int)(w - 2 * radius),
                     (int)h, col);
  ImageDrawRectangle(dst, (int)x, (int)(y + radius), (int)radius,
                     (int)(h - 2 * radius), col);
  ImageDrawRectangle(dst, (int)(x + w - radius), (int)(y + radius), (int)radius,
                     (int)(h - 2 * radius), col);

  // Draw the 4 rounded corners
  ImageDrawCircle(dst, (int)(x + radius), (int)(y + radius), (int)radius, col);
  ImageDrawCircle(dst, (int)(x + w - radius), (int)(y + radius), (int)radius,
                  col);
  ImageDrawCircle(dst, (int)(x + radius), (int)(y + h - radius), (int)radius,
                  col);
  ImageDrawCircle(dst, (int)(x + w - radius), (int)(y + h - radius), (int)radius,
                  col);
}

/*!***************************************************
 * @brief    Draws a text box
 * @details  Draws a text box and handles the word
 * wrapping so that the text doesn't go off the canvas.
 * @param    target Image*
 * @param    font Font
 * @param    text const char*
 * @param    x float
 * @param    y float
 * @param    fontSize float
 * @param    spacing float
 * @param    tint Color
 * @param    maxWidth float
 * @return   float
 * @note
 * @date     2026.01.20
 * @author   bearded.griffin
 ****************************************************/
float DrawTextBox(Image *target, Font font, const char *text, float x, float y,
                  float fontSize, float spacing, Color tint, float maxWidth, float rotation) {
  if (text == nullptr || strlen(text) == 0)
    return 0;

  std::string content(text);
  std::vector<std::string> words;
  std::stringstream ss(content);
  std::string word;

  while (ss >> word) {
    words.push_back(word);
  }

  float currentX = 0;
  float currentY = 0;
  float spaceWidth = MeasureTextEx(font, " ", fontSize, spacing).x;

  for (size_t i = 0; i < words.size(); i++) {
    std::string word = words[i];
    Vector2 wordSize = MeasureTextEx(font, word.c_str(), fontSize, spacing);

    if (maxWidth > 0 && currentX + wordSize.x > maxWidth && currentX > 0) {
      currentX = 0;
      currentY += fontSize;
    }

    Vector2 pos = {currentX, currentY};
    if (rotation != 0) {
        pos = Vector2Rotate(pos, rotation * DEG2RAD);
    }

    if (target) {
      // Note: ImageDrawTextEx does not support rotation directly.
      // For printing, rotation would require a more complex approach (rendering to a separate image then rotating that).
      // For now, we only support 0-degree rotation for printing text boxes.
      ImageDrawTextEx(target, font, word.c_str(), {x + pos.x, y + pos.y},
                      fontSize, spacing, tint);
    } else {
      DrawTextPro(font, word.c_str(), {x + pos.x, y + pos.y}, {0, 0}, rotation, fontSize,
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
                 const Camera2D &camera,
                 const std::vector<int> &selectedIndices) {

  BeginMode2D(camera);

  LabelSize currentSize = LabelSizes[currentProject.selectedLabelIndex];
  DrawRectangle(0, 0, (int)currentSize.width, (int)currentSize.height, WHITE);
  DrawRectangleLines(0, 0, (int)currentSize.width, (int)currentSize.height,
                     GRAY);

  if (Utils::appSettings.showGrid) {
    DrawGrid(currentSize);
  }

  // Object rendering - Use the consolidated RenderObject function
  for (int i = 0; i < currentProject.objects.size(); i++) {
    if (!currentProject.objects[i].isVisible) continue;
    bool isSelected = OBJECTS::IsObjectSelected(selectedIndices, i);
    RenderObject(currentProject.objects[i], isSelected, camera);
  }

  // Draw Snapping Guides
  for (const auto &guide : interactionState.activeGuides) {
    if (guide.isVertical) {
      DrawLineEx({guide.pos, 0}, {guide.pos, currentSize.height}, 1.0f / camera.zoom, ORANGE);
    } else {
      DrawLineEx({0, guide.pos}, {currentSize.width, guide.pos}, 1.0f / camera.zoom, ORANGE);
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