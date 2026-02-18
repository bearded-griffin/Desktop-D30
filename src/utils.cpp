//  This file is part of Desktop-D30
//  Copyright (C) 2026 Chris Griffin (bearded-griffin)
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation version 3 of the License.
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <https://www.gnu.org/licenses/>.

/*!***************************************************
 * @file     src/utils.cpp
 * @brief    Uitity functions used in Desktop-D30
 * @details  Utility functions that allow for saving,
 *  loading, and other general functions.
 * @note
 * @date     2026.01.19
 * @author   bearded.griffin
 ****************************************************/

#include "win_fix.h"
#include "utils.h"

#include <chrono>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <sstream>
#include <thread>

#include "assets.h"
#include "barcode.h"
#include "objects.h"

#include "portable-file-dialogs.h" // Native dialogs

#include "protocol.h"
#include "rendering.h"
#include "types.h"
#include <cstdlib>

namespace Utils {

AppSettings appSettings; // Global instance definition

/*!***************************************************
 * @brief    Saves the project
 * @details  Saves the file as a .json document that
 * represents the various states of the project like
 * the objects and their properties.
 * @param    defaultName const std::string&
 * @param    project const Project&
 * @return   bool if the save was successful
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
    if (destPath.find(".d30") == std::string::npos) {
      destPath += ".d30";
    }
    project.projectFilePath = destPath;
    nlohmann::json j = project;
    std::ofstream file(destPath);
    if (file.is_open()) {
      file << j.dump(4);
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
      
      // Clean up current textures before overwriting the project
      OBJECTS::UnloadProjectObjects(outProject);

      outProject = j.get<Project>();
      outProject.projectFilePath = dest[0]; // Update path to actual file loaded
      outProject.isDirty = false; // A fresh load means no dirty state
      // The csvFilePath should only be set if an actual CSV is loaded, not
      // by loading the project file itself.

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



      return true;
    } catch (...) {
      return false;
    }
  }
  return false;
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
  Image img = RENDERING::RenderProjectToImage(project);

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
 * @brief    Opens a file with the default application
 * @details  Uses system-specific commands to open the
 * file in the default viewer (e.g., xdg-open, open, ShellExecute).
 * @param    filePath const std::string&
 * @return   void
 * @note
 * @date     2026.02.17
 * @author   gemini-cli
 ****************************************************/
void OpenFile(const std::string &filePath) {
#if defined(_WIN32)
  // 1 = SW_SHOWNORMAL
  ShellExecuteA(NULL, "open", filePath.c_str(), NULL, NULL, 1);
#elif defined(__APPLE__)
  std::string command = "open " + filePath;
  system(command.c_str());
#else
  std::string command = "xdg-open " + filePath;
  system(command.c_str());
#endif
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
  if (project.csvRows.empty() || project.currentCSVRow < 0 ||
      project.currentCSVRow >= project.csvRows.size()) {
    return;
  }

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
                Image img = ::LoadImage(obj.data.c_str());
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

void BatchPrint(const Project &project, int startRow, int endRow) {
  for (int i = startRow - 1; i < endRow; i++) {
    // 1. Get Data
    const std::vector<std::string> &rowData = project.csvRows[i];

    // 2. Create Temp Project
    Project tempProject = project;

    // 3. Inject Data
    for (auto &obj : tempProject.objects) {
      if (!obj.linkedColumn.empty()) {
        for (size_t c = 0; c < project.csvHeaders.size(); c++) {
          if (project.csvHeaders[c] == obj.linkedColumn) {
            if (c < rowData.size()) {
              obj.data = rowData[c];
            }
            break;
          }
        }
      }
    }

    // 4. Print
    std::cout << "[Batch] Printing Row " << (i + 1) << "..." << std::endl;
    Protocol::PrintLabel(tempProject);

    // 5. Delay (Prevent buffer overflow)
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }
}

/*!***************************************************
 * @brief    Saves user settings
 * @details  Saves settings like dark mode and grid
 * visibility to a local json file.
 * @param    project const Project&
 * @return   void
 * @note
 * @date     2026.02.02
 * @author   bearded.griffin
 ****************************************************/
void SaveSettings(const AppSettings &settings, const std::string &filename) {
  nlohmann::json j;
  j["darkTheme"] = settings.darkTheme;
  j["showGrid"] = settings.showGrid;

  std::ofstream file(filename);
  if (file.is_open()) {
    file << j.dump(4);
  }
}

/*!***************************************************
 * @brief    Loads user settings
 * @details  Loads settings like dark mode and grid
 * visibility from a local json file.
 * @param    project Project&
 * @return   void
 * @note
 * @date     2026.02.02
 * @author   bearded.griffin
 ****************************************************/
void LoadSettings(AppSettings &settings, const std::string &filename) {
  std::ifstream file(filename);
  if (!file.is_open()) {
    return;
  }
  try {
    nlohmann::json j;
    file >> j;
    if (j.contains("darkTheme")) {
      settings.darkTheme = j["darkTheme"].get<bool>();
    }
    if (j.contains("showGrid")) {
      settings.showGrid = j["showGrid"].get<bool>();
    }
  } catch (...) {
    // Fail silently if settings are corrupt
  }
}

} // namespace Utils