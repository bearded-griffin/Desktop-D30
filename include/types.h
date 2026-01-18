#pragma once
//#include "raylib.h"
#include <string>
#include <vector>
#include <nlohmann/json.hpp> // Required for the JSON macros below

// Define your Object
struct LabelObject {
    // We break out X/Y specifically so they save cleanly to JSON
    // (Raylib's Vector2 is nice, but harder to serialize automatically)
    float x = 0.0f;
    float y = 0.0f;
    std::string text = "New Label";
    float fontSize = 20.0f;
    
    // We'll store color as a hex integer for saving (e.g. 0xFFFFFFFF)
    // You can convert to Raylib Color at runtime
    unsigned int colorHex = 0xFF000000; // Default Black
};

// Define the Label Size
struct LabelSize {
    std::string name;
    float width;
    float height;
};

// Define the Project Container
struct Project {
    int version = 1;
    bool darkTheme = false;
    bool showGrid = true;
    int selectedLabelIndex = 0;
    std::vector<LabelObject> objects;
};

// --- JSON MAGIC ---
// These macros automatically generate to_json() and from_json() functions for you.
// You never have to write parsing code for these fields.

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(LabelObject, x, y, text, fontSize, colorHex)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Project, version, darkTheme, showGrid, selectedLabelIndex, objects)