/*!***************************************************
 * @file     utils.cpp
 * @brief    Uitity functions used in LabelForge
 * @details  Utility functions that allow for saving,
 *  loading, and other general functions.
 * @note
 * @date     2026.01.19
 * @author   bearded.griffin
 ****************************************************/

#include "utils.h"
#include "portable-file-dialogs.h" // Native dialogs
#include "qrcodegen.hpp"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

using namespace qrcodegen;

namespace Utils {

// Holds the label sizes that are the most commonly used.
// TODO: figure out how to handle the circle labels and the continuous labels.
const std::vector<LabelSize> LabelSizes = {
    {"12mm x 30mm", 240, 96},  {"12mm x 40mm", 320, 96},
    {"14mm x 30mm", 240, 112}, {"14mm x 40mm", 320, 112},
    {"14mm x 50mm", 400, 112}, {"15mm x 50mm", 400, 120}};

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
    Vector2 size =
        MeasureTextEx(GetFontDefault(), obj.data.c_str(), obj.fontSize, 2.0f);
    return Rectangle{obj.x, obj.y, size.x, size.y};
  } else {
    // QR Codes and Images use explicit width/height
    // If width is 0 (new object), give it a default
    float w = (obj.width > 0) ? obj.width : 50.0f;
    float h = (obj.height > 0) ? obj.height : 50.0f;
    return Rectangle{obj.x, obj.y, w, h};
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

/*!***************************************************
 * @brief    Saves the project
 * @details  Saves the file as a .json document that
 * represents the various states of the project like
 * the objects and their properties.
 * @param    defaultName const std::string&
 * @param    project const Project&
 * @return   void
 * @note
 * @date     2026.01.19
 * @author   bearded.griffin
 ****************************************************/
void SaveProject(const std::string &defaultName, const Project &project) {
  // Native Save Dialog
  auto dest =
      pfd::save_file("Save Project", defaultName,
                     {"LabelForge Files", "*.flbl"}, pfd::opt::force_overwrite)
          .result();

  if (!dest.empty()) {
    // Ensure extension
    if (dest.find(".flbl") == std::string::npos)
      dest += ".flbl";

    nlohmann::json j = project;
    std::ofstream file(dest);
    if (file.is_open())
      file << j.dump(4);
  }
}

/*!***************************************************
 * @brief    Loads a Project File
 * @details  Opens a previously saved project file and
 * applies all the settings and places the objects back
 * on the canvas where they were at the time of saving.
 * @param    defaultName const std::string&
 * @param    outProject Project&
 * @return   bool if the load was successful
 * @note     
 * @date     2026.01.19
 * @author   bearded.griffin
 ****************************************************/
bool LoadProject(const std::string &defaultName, Project &outProject) {
  // Native Open Dialog
  auto dest = pfd::open_file("Open Project", defaultName,
                             {"ForgeLabel Files", "*.flbl"})
                  .result();

  if (!dest.empty()) {
    std::ifstream file(dest[0]);
    if (!file.is_open())
      return false;
    try {
      nlohmann::json j;
      file >> j;
      outProject = j.get<Project>();
      return true;
    } catch (...) {
      return false;
    }
  }
  return false;
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
    Color col = GetColor(obj.colorHex);

    if (obj.type == ObjectType::Text) {
      ImageDrawTextEx(&canvas, GetFontDefault(), obj.data.c_str(),
                      {obj.x, obj.y}, obj.fontSize, 2.0f, col);
    } else if (obj.type == ObjectType::Field) {
      // Fields are black when printed
      ImageDrawTextEx(&canvas, GetFontDefault(), obj.data.c_str(),
                      {obj.x, obj.y}, obj.fontSize, 2.0f, BLACK);
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
              ImageDrawRectangle(&canvas, px, py, pSize, pSize, col);
            }
          }
        }
      }
    } else if (obj.type == ObjectType::Image) {
      if (FileExists(obj.data.c_str())) {
        Image srcImg = LoadImage(obj.data.c_str());

        // Resize to target dimensions
        ImageResize(&srcImg, (int)obj.width, (int)obj.height);

        // Draw onto canvas
        // Raylib doesn't have ImageDrawImage, but has ImageDraw
        // rect source (entire image) -> rect dest (position on canvas)
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
    }
  }
  return canvas;
}

/*!***************************************************
 * @brief    Creates the image of the label
 * @details  Takes the label and how it is laid out on the
 * canvas, then makes it so it can be sent to the printer.
 * @param    filename const std::string&
 * @param    project const Project&
 * @return   void
 * @note
 * @date     2026.01.20
 * @author   bearded.griffin
 ****************************************************/
void ExportProjectToPNG(const std::string &filename, const Project &project) {
  // Step 1: Generate the pixels
  Image img = RenderProjectToImage(project);

  // Step 2: Save to disk
  std::string finalPath = filename;
  if (finalPath.find(".png") == std::string::npos)
    finalPath += ".png";

  ExportImage(img, finalPath.c_str());

  // Step 3: Cleanup
  UnloadImage(img);
  std::cout << "Exported Label to: " << finalPath << std::endl;
}

} // namespace Utils