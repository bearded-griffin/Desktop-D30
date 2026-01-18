#include "utils.h"
#include <fstream> 
#include <iostream>
#include <nlohmann/json.hpp>

namespace Utils {

    Vector2 GetMouseDeltaWorld(Camera2D camera) {
        Vector2 delta = GetMouseDelta();
        return Vector2Scale(delta, -1.0f / camera.zoom);
    }

    void SaveProject(const std::string& filename, const Project& project) {
        // 1. Convert our Project struct to a JSON object
        // The macro in types.h does all the magic here automatically
        nlohmann::json j = project;

        // 2. Open a file stream for writing
        std::ofstream file(filename);
        if (file.is_open()) {
            // 3. Write it with 4-space indentation (pretty print)
            file << j.dump(4);
            file.close();
            std::cout << "Project saved successfully to " << filename << std::endl;
        } else {
            std::cerr << "Failed to open file for writing: " << filename << std::endl;
        }
    }

    bool LoadProject(const std::string& filename, Project& outProject) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Failed to open file for reading: " << filename << std::endl;
            return false;
        }

        try {
            // 1. Parse the file into a JSON object
            nlohmann::json j;
            file >> j;

            // 2. Convert JSON back into our C++ struct
            // Again, the macro handles the mapping automatically
            outProject = j.get<Project>();
            
            return true;
        } catch (const std::exception& e) {
            std::cerr << "JSON Parsing Error: " << e.what() << std::endl;
            return false;
        }
    }
}