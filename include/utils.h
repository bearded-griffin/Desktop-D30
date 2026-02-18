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
 * @file     include/utils.h
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

// Save/Load Project
bool SaveProject(Project &project, const std::string &filePath = "");
bool LoadProject(const std::string &filename, Project &outProject);
void ExportProjectToPNG(const std::string &filename, const Project &project);

bool LoadCSV(const std::string &filename, Project &project);
void OpenFile(const std::string &filePath);

// Apply the data from project.currentCSVRow to all linked objects
void ApplyCSVDataToObjects(Project &project);
void BatchPrint(const Project &project, int startRow, int endRow);

// --- Local Settings ---
extern AppSettings appSettings;
void SaveSettings(const AppSettings &settings, const std::string &filename = "settings.json");
void LoadSettings(AppSettings &settings, const std::string &filename = "settings.json");

} // namespace Utils