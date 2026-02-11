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
 * @file     src/objects.cpp
 * @brief    Handles all objects
 * @details  Handles object selection, resizing, and dragging
 * @note
 * @date     2026.02.03
 * @author   bearded.griffin
 ****************************************************/

#include "objects.h"
#include "assets.h"
#include "utils.h"

// Object selection handling
namespace OBJECTS {

/*!***************************************************
 * @brief    Handles object selection
 * @details
 * @param    project Project&
 * @param    selectedIndex int&
 * @param    isDraggingObject bool&
 * @param    dragOffset Vector2&
 * @param    mouseWorld const Vector2&
 * @param    camera const Camera2D&
 * @return   void
 * @note
 * @date     2026.02.03
 * @author   bearded.griffin
 ****************************************************/
void HandleObjectSelection(Project &project, int &selectedIndex,
                           bool &isDraggingObject, Vector2 &dragOffset,
                           const Vector2 &mouseWorld, const Camera2D &camera) {
  int clickedIndex = -1;

  // Iterate backwards to select top-most item
  for (int i = project.objects.size() - 1; i >= 0; --i) {
    const auto &obj = project.objects[i];
    if (obj.type == ObjectType::Line) {
      if (CheckCollisionPointLine(
              mouseWorld, {obj.x, obj.y},
              {obj.x + obj.width, obj.y + obj.height},
              static_cast<int>(obj.fontSize / camera.zoom))) {
        clickedIndex = i;
        break;
      }
    } else if (CheckCollisionPointRec(mouseWorld, GetObjectBounds(obj))) {
      clickedIndex = i;
      break;
    }
  }

  selectedIndex = clickedIndex;
  if (selectedIndex != -1) {
    isDraggingObject = true;
    const auto &obj = project.objects[selectedIndex];
    dragOffset = {mouseWorld.x - obj.x, mouseWorld.y - obj.y};
  }
}

/*!***************************************************
 * @brief    Handles Object Resizing
 * @details
 * @param    project Project&
 * @param    selectedIndex int
 * @param    activeHandle ResizeHandle
 * @param    mouseWorld const Vector2&
 * @param    camera const Camera2D&
 * @return   void
 * @note
 * @date     2026.02.03
 * @author   bearded.griffin
 ****************************************************/
void HandleObjectResize(Project &project, const int &selectedIndex,
                        ResizeHandle activeHandle, const Vector2 &mouseWorld,
                        const Camera2D &camera) {
  auto &obj = project.objects[selectedIndex];
  project.isDirty = true;

  if (obj.type == ObjectType::Line) {
    SetMouseCursor(MOUSE_CURSOR_RESIZE_ALL);
    if (activeHandle == HANDLE_TOP_LEFT) {
      float endX = obj.x + obj.width;
      float endY = obj.y + obj.height;
      obj.x = mouseWorld.x;
      obj.y = mouseWorld.y;
      obj.width = endX - obj.x;
      obj.height = endY - obj.y;
    } else if (activeHandle == HANDLE_BOTTOM_RIGHT) {
      obj.width = mouseWorld.x - obj.x;
      obj.height = mouseWorld.y - obj.y;
      if (IsKeyDown(KEY_LEFT_SHIFT)) {
        if (abs(obj.width) > abs(obj.height)) {
          obj.height = 0;
        } else {
          obj.width = 0;
        }
      }
    }
  } else if (obj.type == ObjectType::Text || obj.type == ObjectType::Field) {
    Vector2 mouseDelta = GetMouseDelta();
    obj.fontSize -= (mouseDelta.y * TEXT_RESIZE_FACTOR);
    obj.fontSize = std::max(obj.fontSize, MIN_FONT_SIZE);
  } else {
    float originalWidth = obj.width;
    float originalHeight = obj.height;
    float aspectRatio =
        (originalHeight != 0) ? (originalWidth / originalHeight) : 1.0f;

    Rectangle bounds = GetObjectBounds(obj);
    float handleSize = HANDLE_SIZE / camera.zoom;

    switch (activeHandle) {
    case HANDLE_TOP_LEFT:
      SetMouseCursor(MOUSE_CURSOR_RESIZE_NWSE);
      obj.width = bounds.x + bounds.width - mouseWorld.x;
      obj.height = bounds.y + bounds.height - mouseWorld.y;
      obj.x = mouseWorld.x;
      obj.y = mouseWorld.y;
      if (IsKeyDown(KEY_LEFT_SHIFT) && aspectRatio != 0) {
        obj.height = obj.width / aspectRatio;
        obj.y = (obj.y + originalHeight) - obj.height;
      }
      break;

    case HANDLE_TOP_RIGHT:
      SetMouseCursor(MOUSE_CURSOR_RESIZE_NESW);
      obj.width = mouseWorld.x - obj.x;
      obj.height = bounds.y + bounds.height - mouseWorld.y;
      obj.y = mouseWorld.y;
      if (IsKeyDown(KEY_LEFT_SHIFT) && aspectRatio != 0) {
        obj.height = obj.width / aspectRatio;
        obj.y = (obj.y + originalHeight) - obj.height;
      }
      break;

    case HANDLE_BOTTOM_LEFT:
      SetMouseCursor(MOUSE_CURSOR_RESIZE_NESW);
      obj.width = bounds.x + bounds.width - mouseWorld.x;
      obj.height = mouseWorld.y - obj.y;
      obj.x = mouseWorld.x;
      if (IsKeyDown(KEY_LEFT_SHIFT) && aspectRatio != 0) {
        obj.height = obj.width / aspectRatio;
      }
      break;

    case HANDLE_BOTTOM_RIGHT:
      SetMouseCursor(MOUSE_CURSOR_RESIZE_NWSE);
      obj.width = mouseWorld.x - obj.x;
      obj.height = mouseWorld.y - obj.y;
      if (IsKeyDown(KEY_LEFT_SHIFT) && aspectRatio != 0) {
        obj.height = obj.width / aspectRatio;
      }
      break;

    default:
      break;
    }

    obj.width = std::max(obj.width, MIN_OBJECT_SIZE);
    obj.height = std::max(obj.height, MIN_OBJECT_SIZE);
  }
}

/*!***************************************************
 * @brief    Handles Object Dragging
 * @details
 * @param    project Project&
 * @param    selectedIndex int
 * @param    mouseWorld const Vector2&
 * @param    dragOffset const Vector2&
 * @param    camera const Camera2D&
 * @return   void
 * @note
 * @date     2026.02.03
 * @author   bearded.griffin
 ****************************************************/
void HandleObjectDrag(Project &project, const int &selectedIndex,
                      const Vector2 &mouseWorld, const Vector2 &dragOffset,
                      const Camera2D &camera) {
  auto &obj = project.objects[selectedIndex];
  float newX = mouseWorld.x - dragOffset.x;
  float newY = mouseWorld.y - dragOffset.y;

  Rectangle bounds = GetObjectBounds(obj);
  LabelSize canvasSz = LabelSizes[project.selectedLabelIndex];

  // Clamp position within canvas bounds
  newX = std::max(0.0f, std::min(newX, canvasSz.width - bounds.width));
  newY = std::max(0.0f, std::min(newY, canvasSz.height - bounds.height));

  obj.x = newX;
  obj.y = newY;
  project.isDirty = true;
}

/*!***************************************************
 * @brief    Creates a Text Object
 * @details
 * @param    x float
 * @param    y float
 * @param    text const std::string&
 * @param    fontSize float
 * @param    colorHex unsigned int
 * @return   LabelObject
 * @note
 * @date     2026.02.03
 * @author   bearded.griffin
 ****************************************************/
LabelObject CreateTextObject(const float &x, const float &y,
                             const std::string &text, const float &fontSize) {
  LabelObject obj;
  obj.type = ObjectType::Text;
  obj.x = x;
  obj.y = y;
  obj.width = 0;
  obj.height = 0;
  obj.data = text;
  obj.fontSize = fontSize;
  obj.colorHex = 0x000000FF;
  return obj;
}

/*!***************************************************
 * @brief    Creates a Field Object
 * @details
 * @param    x float
 * @param    y float
 * @param    text const std::string&
 * @param    fontSize float
 * @param    colorHex unsigned int
 * @return   LabelObject
 * @note
 * @date     2026.02.03
 * @author   bearded.griffin
 ****************************************************/
LabelObject CreateFieldObject(const float &x, const float &y,
                              const std::string &text, const float &fontSize) {
  LabelObject obj;
  obj.type = ObjectType::Field;
  obj.x = x;
  obj.y = y;
  obj.width = 0;
  obj.height = 0;
  obj.data = text;
  obj.fontSize = fontSize;
  obj.colorHex = 0x000000FF;
  return obj;
}

/*!***************************************************
 * @brief    Creates a Rectangle Object
 * @details
 * @param    x const float&
 * @param    y const float&
 * @param    width const float&
 * @param    height const float&
 * @param    cornerRadius const float&
 * @return   LabelObject
 * @note
 * @date     2026.02.03
 * @author   bearded.griffin
 ****************************************************/
LabelObject CreateRectangleObject(const float &x, const float &y,
                                  const float &width, const float &height,
                                  const float &cornerRadius) {
  LabelObject obj;
  obj.type = ObjectType::ShapeRect;
  obj.fontSize = 4; // Line thickness
  obj.x = x;
  obj.y = y;
  obj.width = width;
  obj.height = height;
  obj.cornerRadius = cornerRadius;
  obj.colorHex = 0x000000FF;
  return obj;
}

/*!***************************************************
 * @brief    Creates a Circle Object
 * @details
 * @param    x const float&
 * @param    y const float&
 * @param    radius const float&
 * @return   LabelObject
 * @note
 * @date     2026.02.03
 * @author   bearded.griffin
 ****************************************************/
LabelObject CreateCircleObject(const float &x, const float &y,
                               const float &radius) {
  LabelObject obj;
  obj.type = ObjectType::ShapeCircle;
  obj.fontSize = 4; // Line thickness
  obj.x = x;
  obj.y = y;
  obj.width = radius * 2;
  obj.height = radius * 2;
  obj.colorHex = 0x000000FF;
  return obj;
}

/*!***************************************************
 * @brief    Creates a Border Object
 * @details  This is a basic border. Pretty much
 * a rectangle that is placed in the background
 * that the user can change the line thickness and
 * the radius of the corners.
 * @param    x const float&
 * @param    y const float&
 * @param    sz const LabelSize&
 * @param    radius const int&
 * @return   LabelObject
 * @note
 * @date     2026.02.03
 * @author   bearded.griffin
 ****************************************************/
LabelObject CreateBorderObject(const float &x, const float &y,
                               const LabelSize &sz, const int &radius) {
  LabelObject obj;
  obj.type = ObjectType::Border;
  obj.x = x;
  obj.y = y;
  obj.width = (float)sz.width - 8;
  obj.height = (float)sz.height - 8;
  obj.fontSize = 4;
  obj.colorHex = 0x000000FF;
  obj.cornerRadius = radius;
  return obj;
}

/*!***************************************************
 * @brief    Creates a Line Object
 * @details
 * @param    x const float&
 * @param    y const float&
 * @param    width const float&
 * @param    height const float&
 * @param    thickness const float&
 * @return   LabelObject
 * @note
 * @date     2026.02.03
 * @author   bearded.griffin
 ****************************************************/
LabelObject CreateLineObject(const float &x, const float &y, const float &width,
                             const float &height, const float &thickness) {
  LabelObject obj;
  obj.type = ObjectType::Line;
  obj.x = x;
  obj.y = y;
  obj.width = width;
  obj.height = height;
  obj.fontSize = thickness;
  obj.colorHex = 0x000000FF;
  return obj;
}

/*!***************************************************
 * @brief    Creates a QR Code Object
 * @details
 * @param    x const float&
 * @param    y const float&
 * @param    size const float&
 * @param    data const std::string&
 * @return   LabelObject
 * @note
 * @date     2026.02.03
 * @author   bearded.griffin
 ****************************************************/
LabelObject CreateQRCodeObject(const float &x, const float &y,
                               const float &size, const std::string &data) {
  LabelObject obj;
  obj.type = ObjectType::QRCode;
  obj.x = x;
  obj.y = y;
  obj.width = size;
  obj.height = size;
  obj.data = data;
  obj.colorHex = 0x000000FF;
  return obj;
}

/*!***************************************************
 * @brief    Creates a Barcode Object
 * @details
 * @param    x const float&
 * @param    y const float&
 * @param    width const float&
 * @param    height const float&
 * @param    data const std::string&
 * @return   LabelObject
 * @note
 * @date     2026.02.03
 * @author   bearded.griffin
 ****************************************************/
LabelObject CreateBarcodeObject(const float &x, const float &y,
                                const float &width, const float &height,
                                const std::string &data) {
  LabelObject obj;
  obj.type = ObjectType::Barcode;
  obj.x = x;
  obj.y = y;
  obj.width = width;
  obj.height = height;
  obj.data = data;
  obj.colorHex = 0x000000FF;
  return obj;
}

/*!***************************************************
 * @brief    Creates an Image Object
 * @details
 * @param    x const float&
 * @param    y const float&
 * @param    width const float&
 * @param    height const float&
 * @param    filePath const std::string&
 * @return   LabelObject
 * @note
 * @date     2026.02.03
 * @author   bearded.griffin
 ****************************************************/
LabelObject CreateImageObject(const float &x, const float &y,
                              const float &width, const float &height,
                              const std::string &filePath) {
  LabelObject obj;
  obj.type = ObjectType::Image;
  obj.x = x;
  obj.y = y;
  obj.width = width;
  obj.height = height;
  obj.data = filePath;
  obj.colorHex = 0x000000FF;
  return obj;
}

/*!***************************************************
 * @brief    Checks if an object is selected
 * @details
 * @param    project const Project&
 * @param    index int
 * @return   bool
 * @note
 * @date     2026.02.03
 * @author   bearded.griffin
 ****************************************************/
bool IsObjectSelected(const Project &project, int index) {
  return index >= 0 && index < static_cast<int>(project.objects.size());
}

/*!***************************************************
 * @brief    Clamps object position within canvas bounds
 * @details
 * @param    obj LabelObject&
 * @param    canvasSize const LabelSize&
 * @return   void
 * @note
 * @date     2026.02.03
 * @author   bearded.griffin
 ****************************************************/
void ClampObjectPosition(LabelObject &obj, const LabelSize &canvasSize) {
  Rectangle bounds = GetObjectBounds(obj);
  obj.x = std::max(0.0f, std::min(obj.x, canvasSize.width - bounds.width));
  obj.y = std::max(0.0f, std::min(obj.y, canvasSize.height - bounds.height));
}

/*!***************************************************
 * @brief    Validates object size
 * @details
 * @param    obj LabelObject&
 * @return   void
 * @note
 * @date     2026.02.03
 * @author   bearded.griffin
 ****************************************************/
void ValidateObjectSize(LabelObject &obj) {
  obj.width = std::clamp(obj.width, MIN_OBJECT_SIZE, MAX_OBJECT_SIZE);
  obj.height = std::clamp(obj.height, MIN_OBJECT_SIZE, MAX_OBJECT_SIZE);
  if (obj.type == ObjectType::Text || obj.type == ObjectType::Field) {
    obj.fontSize = std::max(obj.fontSize, MIN_FONT_SIZE);
  }
}

/*!***************************************************
 * @brief    Get the object bounds
 * @details  Takes the object passed in and returns
 * it's boundaries.
 * @param    obj const LabelObject&
 * @return   Rectangle
 * @note
 * @date     2026.01.19
 * @author   bearded.griffin
 ****************************************************/
Rectangle GetObjectBounds(const LabelObject &obj) {
  if (obj.type == ObjectType::Text || obj.type == ObjectType::Field) {
    // We use Default Font here for bounds estimation if exact font isn't
    // critical for simple selection Or better: Use actual font
    Font f = AssetManager::Get().GetFont(obj.fontName);

    if (obj.width > 0) {
      return {obj.x, obj.y, obj.width,
              obj.height > 0 ? obj.height : obj.fontSize * 2}; // Wrapped Box
    }

    Vector2 size = MeasureTextEx(f, obj.data.c_str(), obj.fontSize, 2.0f);

    return {obj.x, obj.y, size.x, size.y};

  } else if (obj.type == ObjectType::QRCode ||
             obj.type == ObjectType::Barcode) {
    return {obj.x, obj.y, obj.width, obj.height};
  } else if (obj.type == ObjectType::Image) {
    return {obj.x, obj.y, obj.width, obj.height};
  }
  // HANDLE SHAPES & LINES ---
  else if (obj.type == ObjectType::Line) {
    // For logic, we treat the bounding box as positive width/height
    float w = std::abs(obj.width);
    float h = std::abs(obj.height);

    // If Horizontal Line (h=0), the "Height" is just the thickness
    if (h < obj.fontSize)
      h = obj.fontSize;

    // If Vertical Line (w=0), the "Width" is just the thickness
    if (w < obj.fontSize)
      w = obj.fontSize;

    return {obj.x, obj.y, w, h};
  } else if (obj.type == ObjectType::ShapeRect ||
             obj.type == ObjectType::ShapeCircle) {
    return {obj.x, obj.y, std::abs(obj.width), std::abs(obj.height)};
  }

  return {obj.x, obj.y, 50, 50};
}

} // namespace OBJECTS