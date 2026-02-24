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
 * @file     src/objects.cpp
 * @brief    Handles all objects
 * @details  Handles object selection, resizing, and dragging
 * @note
 * @date     2026.02.03
 ****************************************************/

#include "objects.h"
#include "assets.h"
#include "utils.h"

// Object selection handling
namespace OBJECTS {

/*!***************************************************
 * @brief    Checks collision against a rotated object
 * @param    point Vector2
 * @param    obj const LabelObject&
 * @return   bool
 * @date     2026.02.19
 ****************************************************/
bool CheckCollisionPointRotatedRec(Vector2 point, const LabelObject &obj) {
  float rotation = (obj.type == ObjectType::ShapeCircle) ? 0.0f : obj.rotation;

  if (rotation == 0) {
    return CheckCollisionPointRec(point, GetObjectBounds(obj));
  }

  // Translate point to origin-relative
  Vector2 p = {point.x - obj.x, point.y - obj.y};
  // Rotate point inversely
  p = Vector2Rotate(p, -rotation * DEG2RAD);

  Rectangle localBounds = {0, 0, obj.width, obj.height};
  // Handle text/field special cases where height isn't explicitly stored or is
  // 0
  if (obj.type == ObjectType::Text || obj.type == ObjectType::Field) {
    if (localBounds.height <= 0)
      localBounds.height = obj.fontSize * 1.5f;
    if (localBounds.width <= 0) {
      Font f = AssetManager::Get().GetFont(obj.fontName);
      localBounds.width =
          MeasureTextEx(f, obj.data.c_str(), obj.fontSize, 2.0f).x;
    }
  }

  return CheckCollisionPointRec(p, localBounds);
}

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
 ****************************************************/
void HandleObjectSelection(Project &project, std::vector<int> &selectedIndices,
                           bool &isDraggingObject, Vector2 &dragOffset,
                           const Vector2 &mouseWorld, const Camera2D &camera) {
  int clickedIndex = -1;

  // Iterate backwards to select top-most item
  for (int i = project.objects.size() - 1; i >= 0; --i) {
    const auto &obj = project.objects[i];
    if (obj.isLocked || !obj.isVisible)
      continue;

    if (obj.type == ObjectType::Line) {
      if (CheckCollisionPointLine(
              mouseWorld, {obj.x, obj.y},
              {obj.x + obj.width, obj.y + obj.height},
              static_cast<int>(obj.fontSize / camera.zoom))) {
        clickedIndex = i;
        break;
      }
    } else if (CheckCollisionPointRotatedRec(mouseWorld, obj)) {
      clickedIndex = i;
      break;
    }
  }

  bool isShiftDown = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);

  if (clickedIndex != -1) {
    if (isShiftDown) {
      // Toggle selection
      auto it = std::find(selectedIndices.begin(), selectedIndices.end(),
                          clickedIndex);
      if (it != selectedIndices.end()) {
        selectedIndices.erase(it);
      } else {
        selectedIndices.push_back(clickedIndex);
      }
    } else {
      // If clicked index is already in selection, keep it (allows group drag)
      if (std::find(selectedIndices.begin(), selectedIndices.end(),
                    clickedIndex) == selectedIndices.end()) {
        selectedIndices.clear();
        selectedIndices.push_back(clickedIndex);
      }
    }

    // Only start dragging if shift is NOT down.
    // Shift is for selection, we don't want them jumping around.
    if (!selectedIndices.empty() && !isShiftDown) {
      // Ensure the clicked index is actually in the selection now
      if (std::find(selectedIndices.begin(), selectedIndices.end(),
                    clickedIndex) != selectedIndices.end()) {
        isDraggingObject = true;
        // Use the clicked object for the drag offset base
        const auto &obj = project.objects[clickedIndex];
        dragOffset = {mouseWorld.x - obj.x, mouseWorld.y - obj.y};
      }
    }
  } else if (!isShiftDown) {
    selectedIndices.clear();
  }
}

/*!***************************************************
 * @brief    Handles Object Resizing
 * @details
 * @param    project Project&
 * @param    primaryIndex int
 * @param    activeHandle ResizeHandle
 * @param    mouseWorld const Vector2&
 * @param    camera const Camera2D&
 * @return   void
 * @note
 * @date     2026.02.03
 ****************************************************/
void HandleObjectResize(Project &project, const int &primaryIndex,
                        ResizeHandle activeHandle, const Vector2 &mouseWorld,
                        const Camera2D &camera) {
  if (primaryIndex < 0 || primaryIndex >= static_cast<int>(project.objects.size()))
    return;

  auto &obj = project.objects[primaryIndex];
  if (obj.isLocked)
    return;
  project.isDirty = true;
  obj.boundsDirty = true;

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

namespace {

/**
 * @brief Applies snapping logic (grid and object-to-object) to the target position.
 */
void ApplySnapping(const Project &project, InteractionState &state,
                   float &targetX, float &targetY, int primaryIdx) {
  state.activeGuides.clear();

  // 1. Grid Snapping
  if (Utils::appSettings.snapToGrid) {
    targetX = roundf(targetX / (float)GRID_SIZE) * (float)GRID_SIZE;
    targetY = roundf(targetY / (float)GRID_SIZE) * (float)GRID_SIZE;
  }

  // 2. Object Snapping (Smart Guides)
  if (Utils::appSettings.snapToObjects) {
    const float SNAP_THRESHOLD = 5.0f;
    Rectangle pb = GetObjectBounds(project.objects[primaryIdx]);

    // Points of Interest for the dragged object (relative to its origin x,y)
    float draggedPOIX[] = {targetX, targetX + pb.width / 2.0f,
                           targetX + pb.width};
    float draggedPOIY[] = {targetY, targetY + pb.height / 2.0f,
                           targetY + pb.height};

    for (int i = 0; i < (int)project.objects.size(); i++) {
      if (IsObjectSelected(state.selectedIndices, i))
        continue;
      const auto &other = project.objects[i];
      if (!other.isVisible)
        continue;

      Rectangle ob = GetObjectBounds(other);
      float otherPOIX[] = {ob.x, ob.x + ob.width / 2.0f, ob.x + ob.width};
      float otherPOIY[] = {ob.y, ob.y + ob.height / 2.0f, ob.y + ob.height};

      // Check X snapping
      bool snappedX = false;
      for (int px = 0; px < 3; px++) {
        for (int ox = 0; ox < 3; ox++) {
          if (abs(draggedPOIX[px] - otherPOIX[ox]) < SNAP_THRESHOLD) {
            float snapDelta = otherPOIX[ox] - draggedPOIX[px];
            targetX += snapDelta;
            state.activeGuides.push_back({otherPOIX[ox], true});
            snappedX = true;
            break;
          }
        }
        if (snappedX)
          break;
      }

      // Check Y snapping
      bool snappedY = false;
      for (int py = 0; py < 3; py++) {
        for (int oy = 0; oy < 3; oy++) {
          if (abs(draggedPOIY[py] - otherPOIY[oy]) < SNAP_THRESHOLD) {
            float snapDelta = otherPOIY[oy] - draggedPOIY[py];
            targetY += snapDelta;
            state.activeGuides.push_back({otherPOIY[oy], false});
            snappedY = true;
            break;
          }
        }
        if (snappedY)
          break;
      }
    }
  }
}

} // namespace

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
 ****************************************************/
void HandleObjectDrag(Project &project, InteractionState &state,
                      const Vector2 &mouseWorld, const Camera2D &camera) {
  if (state.selectedIndices.empty())
    return;

  int primaryIdx = state.selectedIndices[0];
  if (primaryIdx < 0 || primaryIdx >= static_cast<int>(project.objects.size()))
    return;

  float targetX = mouseWorld.x - state.dragOffset.x;
  float targetY = mouseWorld.y - state.dragOffset.y;

  // Apply snapping
  ApplySnapping(project, state, targetX, targetY, primaryIdx);

  float deltaX = targetX - project.objects[primaryIdx].x;
  float deltaY = targetY - project.objects[primaryIdx].y;

  LabelSize canvasSz = LabelSizes[project.selectedLabelIndex];

  // Group movement with boundary check
  float minX = 1e9, minY = 1e9, maxX = -1e9, maxY = -1e9;
  for (int idx : state.selectedIndices) {
    if (idx < 0 || idx >= static_cast<int>(project.objects.size()))
      continue;
    Rectangle b = GetObjectBounds(project.objects[idx]);
    minX = std::min(minX, b.x);
    minY = std::min(minY, b.y);
    maxX = std::max(maxX, b.x + b.width);
    maxY = std::max(maxY, b.y + b.height);
  }

  if (minX + deltaX < 0)
    deltaX = -minX;
  if (minY + deltaY < 0)
    deltaY = -minY;
  if (maxX + deltaX > canvasSz.width)
    deltaX = canvasSz.width - maxX;
  if (maxY + deltaY > canvasSz.height)
    deltaY = canvasSz.height - maxY;

  for (int idx : state.selectedIndices) {
    if (idx < 0 || idx >= static_cast<int>(project.objects.size()))
      continue;
    if (project.objects[idx].isLocked)
      continue;
    project.objects[idx].x += deltaX;
    project.objects[idx].y += deltaY;
    project.objects[idx].boundsDirty = true;
  }
  project.isDirty = true;
}

/*!***************************************************
 * @brief    Aligns selected objects
 * @param    project Project&
 * @param    selectedIndices const std::vector<int>&
 * @param    type AlignmentType
 * @param    relativeToCanvas bool
 * @date     2026.02.19
 ****************************************************/
void AlignObjects(Project &project, const std::vector<int> &selectedIndices,
                  AlignmentType type, bool relativeToCanvas) {
  if (selectedIndices.empty())
    return;

  LabelSize canvasSz = LabelSizes[project.selectedLabelIndex];
  project.isDirty = true;

  if (selectedIndices.size() == 1 || relativeToCanvas) {
    // Align relative to canvas
    for (int idx : selectedIndices) {
      auto &obj = project.objects[idx];
      if (obj.isLocked)
        continue;
      Rectangle b = GetObjectBounds(obj);
      switch (type) {
      case ALIGN_LEFT:
        obj.x = 0;
        break;
      case ALIGN_CENTER_H:
        obj.x = (canvasSz.width - b.width) / 2.0f;
        break;
      case ALIGN_RIGHT:
        obj.x = canvasSz.width - b.width;
        break;
      case ALIGN_TOP:
        obj.y = 0;
        break;
      case ALIGN_CENTER_V:
        obj.y = (canvasSz.height - b.height) / 2.0f;
        break;
      case ALIGN_BOTTOM:
        obj.y = canvasSz.height - b.height;
        break;
      }
    }
  } else {
    // Align relative to the first selected object (anchor)
    auto &anchor = project.objects[selectedIndices[0]];
    Rectangle anchorBounds = GetObjectBounds(anchor);

    float anchorCenterX = anchorBounds.x + anchorBounds.width / 2.0f;
    float anchorCenterY = anchorBounds.y + anchorBounds.height / 2.0f;

    for (int i = 1; i < (int)selectedIndices.size(); ++i) {
      auto &obj = project.objects[selectedIndices[i]];
      if (obj.isLocked)
        continue;
      Rectangle b = GetObjectBounds(obj);
      switch (type) {
      case ALIGN_LEFT:
        obj.x = anchorBounds.x;
        break;
      case ALIGN_CENTER_H:
        obj.x = anchorCenterX - (b.width / 2.0f);
        break;
      case ALIGN_RIGHT:
        obj.x = anchorBounds.x + anchorBounds.width - b.width;
        break;
      case ALIGN_TOP:
        obj.y = anchorBounds.y;
        break;
      case ALIGN_CENTER_V:
        obj.y = anchorCenterY - (b.height / 2.0f);
        break;
      case ALIGN_BOTTOM:
        obj.y = anchorBounds.y + anchorBounds.height - b.height;
        break;
      }
    }
  }
}

/*!***************************************************
 * @brief    Distributes selected objects evenly
 * @param    project Project&
 * @param    selectedIndices const std::vector<int>&
 * @param    type DistributionType
 * @date     2026.02.19
 ****************************************************/
void DistributeObjects(Project &project,
                       const std::vector<int> &selectedIndices,
                       DistributionType type) {
  if (selectedIndices.size() < 3)
    return;

  // 1. Filter out locked objects and store indices
  std::vector<int> workingIndices;
  for (int idx : selectedIndices) {
    if (!project.objects[idx].isLocked) {
      workingIndices.push_back(idx);
    }
  }

  if (workingIndices.size() < 3)
    return;

  // 2. Sort indices based on position (center)
  std::sort(workingIndices.begin(), workingIndices.end(), [&](int a, int b) {
    Rectangle boundsA = GetObjectBounds(project.objects[a]);
    Rectangle boundsB = GetObjectBounds(project.objects[b]);
    float centerA = (type == DISTRIBUTE_HORIZONTALLY)
                        ? boundsA.x + boundsA.width / 2.0f
                        : boundsA.y + boundsA.height / 2.0f;
    float centerB = (type == DISTRIBUTE_HORIZONTALLY)
                        ? boundsB.x + boundsB.width / 2.0f
                        : boundsB.y + boundsB.height / 2.0f;
    return centerA < centerB;
  });

  // 3. Calculate spacing
  Rectangle firstBounds =
      GetObjectBounds(project.objects[workingIndices.front()]);
  Rectangle lastBounds =
      GetObjectBounds(project.objects[workingIndices.back()]);

  float startPos = (type == DISTRIBUTE_HORIZONTALLY)
                       ? firstBounds.x + firstBounds.width / 2.0f
                       : firstBounds.y + firstBounds.height / 2.0f;
  float endPos = (type == DISTRIBUTE_HORIZONTALLY)
                     ? lastBounds.x + lastBounds.width / 2.0f
                     : lastBounds.y + lastBounds.height / 2.0f;

  float totalDistance = endPos - startPos;
  float step = totalDistance / (float)(workingIndices.size() - 1);

  // 4. Apply new positions
  for (size_t i = 1; i < workingIndices.size() - 1; ++i) {
    auto &obj = project.objects[workingIndices[i]];
    Rectangle b = GetObjectBounds(obj);
    float newCenter = startPos + (step * i);

    if (type == DISTRIBUTE_HORIZONTALLY) {
      obj.x = newCenter - (b.width / 2.0f);
    } else {
      obj.y = newCenter - (b.height / 2.0f);
    }
  }
  project.isDirty = true;
}

/*!***************************************************
 * @brief    Creates a Text Object
 * @details
 * @param    x float
 * @param    y float
 * @param    text const std::string&
 * @param    fontSize float
 * @return   LabelObject
 * @note
 * @date     2026.02.03
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
 * @return   LabelObject
 * @note
 * @date     2026.02.03
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
  obj.threshold = 128;
  return obj;
}

/*!***************************************************
 * @brief    Adds a new Text object to the project
 * @param    project Project&
 * @param    state InteractionState&
 * @date     2026.02.19
 ****************************************************/
void AddTextObject(Project &project, InteractionState &state) {
  state.PushHistory(project);
  LabelObject obj = CreateTextObject(0, 0, "Text", 20);
  project.objects.push_back(obj);
  project.isDirty = true;
  state.selectedIndices.clear();
  state.selectedIndices.push_back((int)project.objects.size() - 1);
}

/*!***************************************************
 * @brief    Adds a new Field object to the project
 * @param    project Project&
 * @param    state InteractionState&
 * @date     2026.02.19
 ****************************************************/
void AddFieldObject(Project &project, InteractionState &state) {
  state.PushHistory(project);
  LabelObject obj = CreateFieldObject(0, 0, "{Col}", 20);
  project.objects.push_back(obj);
  project.isDirty = true;
  state.selectedIndices.clear();
  state.selectedIndices.push_back((int)project.objects.size() - 1);
}

/*!***************************************************
 * @brief    Adds a new QR Code object to the project
 * @param    project Project&
 * @param    state InteractionState&
 * @date     2026.02.19
 ****************************************************/
void AddQRCodeObject(Project &project, InteractionState &state) {
  state.PushHistory(project);
  LabelObject obj = CreateQRCodeObject(0, 0, 60, "www.example.com");
  project.objects.push_back(obj);
  project.isDirty = true;
  state.selectedIndices.clear();
  state.selectedIndices.push_back((int)project.objects.size() - 1);
}

/*!***************************************************
 * @brief    Adds a new Barcode object to the project
 * @param    project Project&
 * @param    state InteractionState&
 * @date     2026.02.19
 ****************************************************/
void AddBarcodeObject(Project &project, InteractionState &state) {
  state.PushHistory(project);
  LabelObject obj = CreateBarcodeObject(50, 50, 100, 60, "12345678");
  project.objects.push_back(obj);
  project.isDirty = true;
  state.selectedIndices.clear();
  state.selectedIndices.push_back((int)project.objects.size() - 1);
}

/*!***************************************************
 * @brief    Adds a new Line object to the project
 * @param    project Project&
 * @param    state InteractionState&
 * @date     2026.02.19
 ****************************************************/
void AddLineObject(Project &project, InteractionState &state) {
  state.PushHistory(project);
  LabelObject obj = CreateLineObject(0, 0, 100, 0, 4);
  project.objects.push_back(obj);
  project.isDirty = true;
  state.selectedIndices.clear();
  state.selectedIndices.push_back((int)project.objects.size() - 1);
}

/*!***************************************************
 * @brief    Adds a new Rectangle object to the project
 * @param    project Project&
 * @param    state InteractionState&
 * @date     2026.02.19
 ****************************************************/
void AddRectangleObject(Project &project, InteractionState &state) {
  state.PushHistory(project);
  LabelObject obj = CreateRectangleObject(0, 0, 50, 50, 4);
  project.objects.push_back(obj);
  project.isDirty = true;
  state.selectedIndices.clear();
  state.selectedIndices.push_back((int)project.objects.size() - 1);
}

/*!***************************************************
 * @brief    Adds a new Circle object to the project
 * @param    project Project&
 * @param    state InteractionState&
 * @date     2026.02.19
 ****************************************************/
void AddCircleObject(Project &project, InteractionState &state) {
  state.PushHistory(project);
  LabelObject obj = CreateCircleObject(0, 0, 25);
  project.objects.push_back(obj);
  project.isDirty = true;
  state.selectedIndices.clear();
  state.selectedIndices.push_back((int)project.objects.size() - 1);
}

/*!***************************************************
 * @brief    Adds a new Border object to the project
 * @param    project Project&
 * @param    state InteractionState&
 * @date     2026.02.19
 ****************************************************/
void AddBorderObject(Project &project, InteractionState &state) {
  state.PushHistory(project);
  LabelSize sz = LabelSizes[project.selectedLabelIndex];
  LabelObject obj = CreateBorderObject(4, 4, sz);
  project.objects.insert(project.objects.begin(), obj);
  project.isDirty = true;
  state.selectedIndices.clear();
  state.selectedIndices.push_back(0);
}

/*!***************************************************
 * @brief    Checks if an object is selected
 * @details
 * @param    selectedIndices const std::vector<int>&
 * @param    index int
 * @return   bool
 * @date     2026.02.03
 ****************************************************/
bool IsObjectSelected(const std::vector<int> &selectedIndices, int index) {
  return std::find(selectedIndices.begin(), selectedIndices.end(), index) !=
         selectedIndices.end();
}

/*!***************************************************
 * @brief    Gets the primary (last) selection index
 * @param    selectedIndices const std::vector<int>&
 * @return   int
 * @date     2026.02.19
 ****************************************************/
int GetPrimarySelection(const std::vector<int> &selectedIndices) {
  return selectedIndices.empty() ? -1 : selectedIndices.back();
}

/*!***************************************************
 * @brief    Clamps object position within canvas bounds
 * @details
 * @param    obj LabelObject&
 * @param    canvasSize const LabelSize&
 * @return   void
 * @note
 * @date     2026.02.03
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
 ****************************************************/
void ValidateObjectSize(LabelObject &obj) {
  obj.width = std::clamp(obj.width, MIN_OBJECT_SIZE, MAX_OBJECT_SIZE);
  obj.height = std::clamp(obj.height, MIN_OBJECT_SIZE, MAX_OBJECT_SIZE);
  if (obj.type == ObjectType::Text || obj.type == ObjectType::Field) {
    obj.fontSize = std::max(obj.fontSize, MIN_FONT_SIZE);
  }
}

/*!***************************************************
 * @brief    Unloads all project objects
 * @param    project Project&
 * @return   void
 * @note
 * @date     2026.02.03
 ****************************************************/
void UnloadProjectObjects(Project &project) {
  for (auto &obj : project.objects) {
    if (obj.texture.id != 0) {
      UnloadTexture(obj.texture);
      obj.texture = {0};
    }
    if (obj.hasOriginalImage) {
      UnloadImage(obj.originalImage);
      obj.originalImage = {0};
      obj.hasOriginalImage = false;
    }
  }
  project.objects.clear();
}

/*!***************************************************
 * @brief    Get the object bounds
 * @details  Takes the object passed in and returns
 * it's boundaries.
 * @param    obj const LabelObject&
 * @return   Rectangle
 * @note
 * @date     2026.01.19
 ****************************************************/
Rectangle GetObjectBounds(const LabelObject &obj) {
  LabelObject &mutableObj = const_cast<LabelObject &>(obj);

  if (!obj.boundsDirty) {
    return obj.cachedBounds;
  }

  Rectangle bounds = {obj.x, obj.y, 50, 50};

  if (obj.type == ObjectType::Text || obj.type == ObjectType::Field) {
    Font f;
    if (obj.cachedFont) {
      FontAsset *fa = static_cast<FontAsset *>(obj.cachedFont);
      if (!fa->isLoaded) {
        f = AssetManager::Get().GetFont(obj.fontName);
      } else {
        f = fa->font;
      }
    } else {
      f = AssetManager::Get().GetFont(obj.fontName);
    }

    if (obj.width > 0) {
      bounds = {obj.x, obj.y, obj.width,
                obj.height > 0 ? obj.height : obj.fontSize * 2}; // Wrapped Box
    } else {
      Vector2 size = MeasureTextEx(f, obj.data.c_str(), obj.fontSize, 2.0f);
      bounds = {obj.x, obj.y, size.x, size.y};
    }
  } else if (obj.type == ObjectType::QRCode ||
             obj.type == ObjectType::Barcode || obj.type == ObjectType::Image) {
    bounds = {obj.x, obj.y, obj.width, obj.height};
  } else if (obj.type == ObjectType::Line) {
    float w = std::abs(obj.width);
    float h = std::abs(obj.height);
    if (h < obj.fontSize)
      h = obj.fontSize;
    if (w < obj.fontSize)
      w = obj.fontSize;
    bounds = {obj.x, obj.y, w, h};
  } else if (obj.type == ObjectType::ShapeRect ||
             obj.type == ObjectType::ShapeCircle) {
    bounds = {obj.x, obj.y, std::abs(obj.width), std::abs(obj.height)};
  }

  mutableObj.cachedBounds = bounds;
  mutableObj.boundsDirty = false;
  return bounds;
}

} // namespace OBJECTS