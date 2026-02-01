/*!***************************************************
 * @file     utils.h
 * @brief    Uitity functions used in Desktop-D30
 * @details  Utility functions that allow for saving,
 *  loading, and other general functions.
 * @note
 * @date     2026.01.19
 * @author   bearded.griffin
 ****************************************************/

#pragma once
#include <vector>

#include "raylib.h"
#include "raymath.h"
#include "types.h"

namespace Utils {
// A Global list of available label sizes
extern const std::vector<LabelSize> LabelSizes;

// Helper: Calculates the rectangle (x,y,w,h) for a text object
// We need this for hit-testing and drawing selection boxes
Rectangle GetObjectBounds(const LabelObject &obj);

Vector2 GetMouseDeltaWorld(Camera2D camera);
// Save/Load Project
bool SaveProject(Project &project, const std::string &filePath = "");
bool LoadProject(const std::string &filename, Project &outProject);
void ExportProjectToPNG(const std::string &filename, const Project &project);
void DrawQRCode(const std::string &text, float x, float y, float size,
                Color color);
float DrawTextBox(Image *target, Font font, const char *text, float x, float y,
                  float fontSize, float spacing, Color tint, float maxWidth);

Image RenderProjectToImage(const Project &project);

bool LoadCSV(const std::string &filename, Project &project);

// Apply the data from project.currentCSVRow to all linked objects
void ApplyCSVDataToObjects(Project &project);

// --- Local Settings ---
void SaveSettings(const Project &project);
void LoadSettings(Project &project);

}  // namespace Utils