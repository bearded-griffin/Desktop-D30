/*!***************************************************
 * @file     utils.h
 * @brief    Uitity functions used in LabelForge
 * @details  Utility functions that allow for saving,
 *  loading, and other general functions.
 * @note     
 * @date     2026.01.19
 * @author   bearded.griffin
 ****************************************************/

#pragma once
#include "raylib.h"
#include "raymath.h"
#include "types.h"
#include <vector>

namespace Utils {
// A Global list of available label sizes
extern const std::vector<LabelSize> LabelSizes;

// Helper: Calculates the rectangle (x,y,w,h) for a text object
// We need this for hit-testing and drawing selection boxes
Rectangle GetObjectBounds(const LabelObject &obj);

Vector2 GetMouseDeltaWorld(Camera2D camera);
void SaveProject(const std::string &filename, const Project &project);
bool LoadProject(const std::string &filename, Project &outProject);
void ExportProjectToPNG(const std::string& filename, const Project& project);
void DrawQRCode(const std::string &text, float x, float y, float size,
                Color color);

Image RenderProjectToImage(const Project& project);

bool LoadCSV(const std::string& filename, Project& project);

// Apply the data from project.currentCSVRow to all linked objects
void ApplyCSVDataToObjects(Project& project);
                
}