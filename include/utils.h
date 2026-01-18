#pragma once
#include "raylib.h"
#include "raymath.h"
#include "types.h"
#include <string>
#include <vector>

namespace Utils {
    // Helper to get mouse delta in World Space
    Vector2 GetMouseDeltaWorld(Camera2D camera);

    // Save the list of objects to a JSON file
    void SaveProject(const std::string& filename, const Project& project);

    // Load a JSON file and return a Project struct
    // We return a "bool" to indicate success/failure, and fill the project by reference
    bool LoadProject(const std::string& filename, Project& outProject);
}