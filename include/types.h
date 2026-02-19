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
 * @file     include/types.h
 * @brief    The various types of objects used by Desktop-D30
 * @details  All the things...objects atleast.
 * @note
 * @date     2026.01.19
 * @author   bearded.griffin
 ****************************************************/

#pragma once
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "raylib.h"

// Constants
constexpr float HANDLE_RADIUS = 10.0f;
constexpr float HANDLE_SIZE = 12.0f;
constexpr float MIN_ZOOM = 0.1f;
constexpr float MIN_OBJECT_SIZE = 10.0f;
constexpr float MAX_OBJECT_SIZE = 1000.0f;
constexpr float MIN_FONT_SIZE = 8.0f;
constexpr float TEXT_RESIZE_FACTOR = 0.2f;
constexpr float ZOOM_SPEED = 0.1f;
constexpr int GRID_SIZE = 20;
constexpr int BARCODE_MODULE_EXTRA = 1;
constexpr int SCREEN_WIDTH = 1280;
constexpr int SCREEN_HEIGHT = 800;

// Define the types of objects we can have
enum class ObjectType {
  Text,
  QRCode,
  Image,
  Field, // For CSV batch printing
  Line,
  ShapeRect,
  ShapeCircle,
  Barcode,
  Border
};

struct LabelSize {
  std::string name;
  float width;
  float height;
};

// A Global list of available label sizes
// extern const std::vector<LabelSize> LabelSizes;
const std::vector<LabelSize> LabelSizes = {
    {"12mm x 30mm", 240, 96},  {"12mm x 40mm", 320, 96},
    {"14mm x 30mm", 240, 112}, {"14mm x 40mm", 320, 112},
    {"14mm x 50mm", 400, 112}, {"15mm x 50mm", 400, 120}};

struct LabelObject {
  // Common Properties
  ObjectType type = ObjectType::Text;
  float x = 0.0f;
  float y = 0.0f;
  float width = 0.0f;  // Used for Image/QR resizing
  float height = 0.0f; // Used for Image/QR resizing

  // Content
  // For Text: The text string
  // For QRCode: The data to encode
  // For Image: The file path
  // For Field: The CSV column header name
  std::string data = "New Object";

  // Data Binding for CSV data
  std::string linkedColumn = "";

  // Style
  std::string fontName = "";
  float fontSize = 20.0f;
  unsigned int colorHex = 0x000000FF;

  float cornerRadius = 0.0f; // 0.0 = Sharp corners

  // --- State ---
  bool isLocked = false;
  bool isVisible = true;
  float rotation = 0.0f; // In degrees
  int threshold = 128; // 0-255 for image binarization

  // --- Auto-Increment ---
  bool isAutoIncrement = false;
  int autoStart = 1;
  int autoStep = 1;
  int autoCurrent = 1;
  std::string autoPrefix = "";
  std::string autoSuffix = "";

  // --- Runtime Texture Resource ---
  Texture2D texture = {0};
  std::string lastData = "";      // For cache tracking
  unsigned int lastColor = 0;     // For cache tracking
  int lastThreshold = -1;         // For cache tracking
};

struct AppSettings {
  bool darkTheme = false;
  bool showGrid = true;
  bool snapToGrid = true;
  bool snapToObjects = true;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(AppSettings, darkTheme, showGrid, snapToGrid, snapToObjects)

struct Project {
  int version = 1;
  int selectedLabelIndex = 0;
  std::string projectFilePath = "";
  std::vector<LabelObject> objects;

  // --- CSV Data ---
  // Headers: ["Name", "Price", "SKU"]
  std::vector<std::string> csvHeaders;
  // Rows: [ ["Widget", "$10", "123"], ["Gadget", "$5", "456"] ]
  std::vector<std::vector<std::string>> csvRows;
  std::string csvFilePath = "";

  // Used for Data Navigation
  int currentCSVRow = 0;

  // --- Runtime State (Not Saved) ---
  bool isDirty = false;
};

enum ResizeHandle {
  HANDLE_NONE,
  HANDLE_TOP_LEFT,
  HANDLE_TOP_RIGHT,
  HANDLE_BOTTOM_LEFT,
  HANDLE_BOTTOM_RIGHT,
  HANDLE_TOP,
  HANDLE_BOTTOM,
  HANDLE_LEFT,
  HANDLE_RIGHT
};

enum AlignmentType {
  ALIGN_LEFT,
  ALIGN_CENTER_H,
  ALIGN_RIGHT,
  ALIGN_TOP,
  ALIGN_CENTER_V,
  ALIGN_BOTTOM
};

enum DistributionType {
  DISTRIBUTE_HORIZONTALLY,
  DISTRIBUTE_VERTICALLY
};

struct SnapGuide {
  float pos;
  bool isVertical;
};

struct InteractionState {
  std::vector<int> selectedIndices;
  bool isDraggingObject = false;
  Vector2 dragOffset = {0, 0};
  bool isResizing = false;
  ResizeHandle activeHandle = HANDLE_NONE;

  // --- Snapping ---
  std::vector<SnapGuide> activeGuides;

  // --- Undo/Redo/Copy/Paste ---
  std::vector<Project> undoStack;
  std::vector<Project> redoStack;
  std::vector<LabelObject> clipboard;

  void PushHistory(const Project &project) {
    undoStack.push_back(project);
    if (undoStack.size() > 50) {
      undoStack.erase(undoStack.begin());
    }
    redoStack.clear();
  }

  void Undo(Project &project) {
    if (undoStack.empty())
      return;
    redoStack.push_back(project);
    project = undoStack.back();
    undoStack.pop_back();
  }

  void Redo(Project &project) {
    if (redoStack.empty())
      return;
    undoStack.push_back(project);
    project = redoStack.back();
    redoStack.pop_back();
  }
};

// JSON Serialization Macros
NLOHMANN_JSON_SERIALIZE_ENUM(ObjectType, {{ObjectType::Text, "text"},
                                          {ObjectType::QRCode, "qrcode"},
                                          {ObjectType::Image, "image"},
                                          {ObjectType::Field, "field"},
                                          {ObjectType::Line, "line"},
                                          {ObjectType::ShapeRect, "rectangle"},
                                          {ObjectType::ShapeCircle, "circle"},
                                          {ObjectType::Barcode, "barcode"},
                                          {ObjectType::Border, "border"}})

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(LabelObject, type, x, y, width, height, data,
                                   linkedColumn, fontName, fontSize, colorHex,
                                   cornerRadius, isLocked, isVisible, rotation, threshold,
                                   isAutoIncrement, autoStart, autoStep, autoCurrent, autoPrefix, autoSuffix)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Project, version, selectedLabelIndex, objects, csvFilePath,
                                   currentCSVRow, projectFilePath, csvHeaders, csvRows)