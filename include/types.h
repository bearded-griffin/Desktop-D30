/*!***************************************************
 * @file     types.h
 * @brief    The various types of objects used by LabelForge
 * @details  All the things...objects atleast.
 * @note
 * @date     2026.01.19
 * @author   bearded.griffin
 ****************************************************/

#pragma once
#include "raylib.h"
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

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

  float cornerRadius = 0.0f;  // 0.0 = Sharp corners

  // --- Runtime Texture Resource ---
  Texture2D texture = {0};
};

struct Project {
  int version = 1;
  bool darkTheme = false; // Checkbox state
  bool showGrid = true;   // Checkbox state
  int selectedLabelIndex = 0;
  std::vector<LabelObject> objects;

  // --- CSV Data ---
  // Headers: ["Name", "Price", "SKU"]
  std::vector<std::string> csvHeaders;
  // Rows: [ ["Widget", "$10", "123"], ["Gadget", "$5", "456"] ]
  std::vector<std::vector<std::string>> csvRows;
  std::string csvFilePath = "";

  // Used for Data Navigation
  int currentCSVRow = 0;
};

// JSON Serialization Macros
NLOHMANN_JSON_SERIALIZE_ENUM(ObjectType, {{ObjectType::Text, "text"},
                                          {ObjectType::QRCode, "qrcode"},
                                          {ObjectType::Image, "image"},
                                          {ObjectType::Field, "field"},
                                          {ObjectType::Line, "line"},
                                          {ObjectType::ShapeRect, "rectangle"},
                                          {ObjectType::ShapeCircle, "circle"},
                                          {ObjectType::Barcode, "barcode"}})

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(LabelObject, type, x, y, width, height, data,
                                   linkedColumn, fontName, fontSize, colorHex, cornerRadius)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Project, version, darkTheme, showGrid,
                                   selectedLabelIndex, objects, csvFilePath,
                                   currentCSVRow)