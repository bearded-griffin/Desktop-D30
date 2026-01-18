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
    Rectangle GetObjectBounds(const LabelObject& obj);

    Vector2 GetMouseDeltaWorld(Camera2D camera);
    void SaveProject(const std::string& filename, const Project& project);
    bool LoadProject(const std::string& filename, Project& outProject);
}