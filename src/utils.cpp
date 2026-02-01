/*!***************************************************
 * @file     utils.cpp
 * @brief    Uitity functions used in Desktop-D30
 * @details  Utility functions that allow for saving,
 *  loading, and other general functions.
 * @note
 * @date     2026.01.19
 * @author   bearded.griffin
 ****************************************************/

#include "utils.h"

#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <sstream>

#include "assets.h"
#include "barcode.h"
#include "portable-file-dialogs.h" // Native dialogs
#include "qrcodegen.hpp"
#include "types.h"

using namespace qrcodegen;

namespace Utils {

// Holds the label sizes that are the most commonly used.
// TODO: figure out how to handle the circle labels and the continuous labels.
const std::vector<LabelSize> LabelSizes = {
    {"12mm x 30mm", 240, 96},  {"12mm x 40mm", 320, 96},
    {"14mm x 30mm", 240, 112}, {"14mm x 40mm", 320, 112},
    {"14mm x 50mm", 400, 112}, {"15mm x 50mm", 400, 120}};

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
bool SaveProject(Project &project, const std::string &filePath) {
  std::string destPath = filePath;

  if (destPath.empty()) {
    // Native Save Dialog
    destPath = pfd::save_file("Save Project", "project.d30",
                              {"Desktop-D30 Files", "*.d30"},
                              pfd::opt::force_overwrite)
                   .result();
  }

  if (!destPath.empty()) {
    // Ensure extension
    if (destPath.find(".d30") == std::string::npos)
      destPath += ".d30";

    nlohmann::json j = project;
    std::ofstream file(destPath);
    if (file.is_open()) {
      file << j.dump(4);
      project.csvFilePath = destPath; // Update project's saved path
      std::cout << "[Utils] Project saved to: " << destPath << std::endl;
      return true;
    }
  }
  return false;
}

/*!***************************************************
 * @brief    Loads a Project File
 * @details  Opens a previously saved project file and
 * applies all the settings and places the objects back
 * on the canvas where they were at the time of saving.
 * If a csv file path is saved, it will be loaded again.
 * @param    defaultName const std::string&
 * @param    outProject Project&
 * @return   bool if the load was successful
 * @note
 * @date     2026.01.19
 * @author   bearded.griffin
 ****************************************************/
bool LoadProject(const std::string &defaultName, Project &outProject) {
  auto dest = pfd::open_file("Open Project", defaultName,
                             {"Desktop-D30 Files", "*.d30"})
                  .result();

  if (!dest.empty()) {
    std::ifstream file(dest[0]);
    if (!file.is_open())
      return false;
    try {
      nlohmann::json j;
      file >> j;
      outProject = j.get<Project>();
      outProject.isDirty = false;       // A fresh load means no dirty state
      outProject.csvFilePath = dest[0]; // Update the project's own file path

      // --- RELOAD CSV DATA ---
      if (!outProject.csvFilePath.empty()) {
        // Check if file still exists
        if (FileExists(outProject.csvFilePath.c_str())) {
          LoadCSV(outProject.csvFilePath, outProject);

          // Re-apply the data for the saved 'currentCSVRow'
          ApplyCSVDataToObjects(outProject);

          std::cout << "[Utils] Auto-reloaded CSV: " << outProject.csvFilePath
                    << std::endl;
        } else {
          std::cout << "[Utils] Warning: Saved CSV file not found: "
                    << outProject.csvFilePath << std::endl;
          // Optional: Clear the path so we don't keep trying
          outProject.csvFilePath = "";
        }
      }

      // Persist the loaded settings as the new default
      SaveSettings(outProject);

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

        // --- CONVERT TO B&W ---
        ImageColorGrayscale(&srcImg);
        // Manual Thresholding
        Color *pixels = LoadImageColors(srcImg);
        for (int i = 0; i < srcImg.width * srcImg.height; i++) {
          if (pixels[i].r < 128) {
            pixels[i] = BLACK;
          } else {
            pixels[i] = WHITE;
          }
        }
        UnloadImageColors(pixels);

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

/*!***************************************************
 * @brief    Load the CSV file
 * @details  parses the provided .csv file so that it
 * can be used to display the data on Label Objects.
 * @param    filename const std::string&
 * @param    project Project&
 * @return   bool If it loaded or not
 * @note
 * @date     2026.01.22
 * @author   bearded.griffin
 ****************************************************/
bool LoadCSV(const std::string &filename, Project &project) {
  std::ifstream file(filename);
  if (!file.is_open())
    return false;

  project.csvHeaders.clear();
  project.csvRows.clear();
  project.csvFilePath = filename;

  std::string line;
  bool isHeader = true;

  while (std::getline(file, line)) {
    // Super simple CSV parser (does not handle quoted commas correctly, but
    // fine for basic usage)
    std::vector<std::string> row;
    std::stringstream ss(line);
    std::string cell;

    while (std::getline(ss, cell, ',')) {
      // Remove carriage returns if any (Windows formatting)
      if (!cell.empty() && cell.back() == '\r')
        cell.pop_back();
      row.push_back(cell);
    }

    if (isHeader) {
      project.csvHeaders = row;
      isHeader = false;
    } else {
      if (row.size() == project.csvHeaders.size()) {
        project.csvRows.push_back(row);
      }
    }
  }
  return true;
}

/*!***************************************************
 * @brief    Applies the current row to the label.
 * @details  It takes the data from the current row that
 * the navigator is currently on and maps it to the fields.
 * @param    project Project&
 * @return   void
 * @note
 * @date     2026.01.23
 * @author   bearded.griffin
 ****************************************************/
void ApplyCSVDataToObjects(Project &project) {
  if (project.csvRows.empty())
    return;

  // Safety Clamp
  if (project.currentCSVRow < 0)
    project.currentCSVRow = 0;
  if (project.currentCSVRow >= project.csvRows.size())
    project.currentCSVRow = (int)project.csvRows.size() - 1;

  // Get the data for the current row
  const std::vector<std::string> &rowData =
      project.csvRows[project.currentCSVRow];

  for (auto &obj : project.objects) {
    // Only update if this object is linked to a column
    if (!obj.linkedColumn.empty()) {
      // Find which column index matches the header name
      for (size_t colIdx = 0; colIdx < project.csvHeaders.size(); colIdx++) {
        if (project.csvHeaders[colIdx] == obj.linkedColumn) {
          // If we have data for this column, update the object
          if (colIdx < rowData.size()) {
            obj.data = rowData[colIdx];

            // Special Case: If it's an Image, we need to reload the texture!
            if (obj.type == ObjectType::Image) {
              if (obj.texture.id != 0)
                UnloadTexture(obj.texture);

              if (FileExists(obj.data.c_str())) {
                Image img = LoadImage(obj.data.c_str());
                // Auto-size if zero
                if (obj.width == 0)
                  obj.width = (float)img.width;
                if (obj.height == 0)
                  obj.height = (float)img.height;

                obj.texture = LoadTextureFromImage(img);
                UnloadImage(img);
              }
            }
          }
          break; // Stop searching headers
        }
      }
    }
  }
}

// --- Local Settings ---
void SaveSettings(const Project &project) {
  nlohmann::json j;
  j["darkTheme"] = project.darkTheme;
  j["showGrid"] = project.showGrid;

  std::ofstream file("settings.json");
  if (file.is_open()) {
    file << j.dump(4);
  }
}

void LoadSettings(Project &project) {
  std::ifstream file("settings.json");
  if (!file.is_open()) {
    return;
  }
  try {
    nlohmann::json j;
    file >> j;
    if (j.contains("darkTheme")) {
      project.darkTheme = j["darkTheme"].get<bool>();
    }
    if (j.contains("showGrid")) {
      project.showGrid = j["showGrid"].get<bool>();
    }
  } catch (...) {
    // Fail silently if settings are corrupt
  }
}

}  // namespace Utils