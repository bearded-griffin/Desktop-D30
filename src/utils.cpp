#include "utils.h"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include "portable-file-dialogs.h" // Native dialogs
#include "qrcodegen.hpp"

using namespace qrcodegen;

namespace Utils {

    const std::vector<LabelSize> LabelSizes = {
        { "12mm x 30mm", 240, 96 },
        { "12mm x 40mm", 320, 96 },
        { "14mm x 30mm", 240, 112 },
        { "14mm x 40mm", 320, 112 },
        { "14mm x 50mm", 400, 112 },
        { "15mm x 50mm", 400, 120 }
    };

    Rectangle GetObjectBounds(const LabelObject& obj) {
        if (obj.type == ObjectType::Text || obj.type == ObjectType::Field) {
            Vector2 size = MeasureTextEx(GetFontDefault(), obj.data.c_str(), obj.fontSize, 2.0f);
            return Rectangle{ obj.x, obj.y, size.x, size.y };
        } else {
            // QR Codes and Images use explicit width/height
            // If width is 0 (new object), give it a default
            float w = (obj.width > 0) ? obj.width : 50.0f;
            float h = (obj.height > 0) ? obj.height : 50.0f;
            return Rectangle{ obj.x, obj.y, w, h };
        }
    }

    Vector2 GetMouseDeltaWorld(Camera2D camera) {
        Vector2 delta = GetMouseDelta();
        return Vector2Scale(delta, -1.0f / camera.zoom);
    }

    void SaveProject(const std::string& defaultName, const Project& project) {
        // Native Save Dialog
        auto dest = pfd::save_file("Save Project", defaultName, { "JSON Files", "*.json" }, pfd::opt::force_overwrite).result();
        
        if (!dest.empty()) {
            // Ensure extension
            if (dest.find(".json") == std::string::npos) dest += ".json";
            
            nlohmann::json j = project;
            std::ofstream file(dest);
            if (file.is_open()) file << j.dump(4);
        }
    }

    bool LoadProject(const std::string& defaultName, Project& outProject) {
        // Native Open Dialog
        auto dest = pfd::open_file("Open Project", defaultName, { "JSON Files", "*.json" }).result();
        
        if (!dest.empty()) {
            std::ifstream file(dest[0]);
            if (!file.is_open()) return false;
            try {
                nlohmann::json j;
                file >> j;
                outProject = j.get<Project>();
                return true;
            } catch (...) { return false; }
        }
        return false;
    }

    void DrawQRCode(const std::string& text, float x, float y, float size, Color color) {
        if (text.empty()) return;

        // 1. Generate the QR Data
        // Ecc::MEDIUM allows for ~15% error correction (good for printing)
        QrCode qr = QrCode::encodeText(text.c_str(), QrCode::Ecc::MEDIUM);

        // 2. Calculate drawing metrics
        int gridSize = qr.getSize(); // e.g., 21, 25, etc.
        if (gridSize <= 0) return;

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
                        (int)(moduleSize + 1), // +1 to fix tiny gaps between floating point rects
                        (int)(moduleSize + 1), 
                        color
                    );
                }
            }
        }
    }
}