#include "utils.h"
#include <fstream>
//#include <iostream>
#include <nlohmann/json.hpp>

namespace Utils {

    // 8 dots per mm
    const std::vector<LabelSize> LabelSizes = {
        { "12mm x 30mm", 240, 96 },
        { "12mm x 40mm", 320, 96 },
        { "14mm x 30mm", 240, 112 },
        { "14mm x 40mm", 320, 112 },
        { "14mm x 50mm", 400, 112 },
        { "15mm x 50mm", 400, 120 }
    };

    Rectangle GetObjectBounds(const LabelObject& obj) {
        // Measure the text using Raylib's default font (or your custom font)
        Vector2 size = MeasureTextEx(GetFontDefault(), obj.text.c_str(), obj.fontSize, 2.0f);
        return Rectangle{ obj.x, obj.y, size.x, size.y };
    }

    Vector2 GetMouseDeltaWorld(Camera2D camera) {
        Vector2 delta = GetMouseDelta();
        return Vector2Scale(delta, -1.0f / camera.zoom);
    }

    // ... (Keep your existing SaveProject/LoadProject implementations here) ...
    void SaveProject(const std::string& filename, const Project& project) {
        nlohmann::json j = project;
        std::ofstream file(filename);
        if (file.is_open()) file << j.dump(4);
    }

    bool LoadProject(const std::string& filename, Project& outProject) {
        std::ifstream file(filename);
        if (!file.is_open()) return false;
        try {
            nlohmann::json j;
            file >> j;
            outProject = j.get<Project>();
            return true;
        } catch (...) { return false; }
    }
}