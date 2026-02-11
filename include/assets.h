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
 * @file     include/assets.h
 * @brief    Supporting functions for icons
 * @details  Creates the Icon Library from the file
 * system.
 * @note
 * @date     2026.01.23
 * @author   bearded.griffin
 ****************************************************/

#pragma once
#include "raylib.h"
#include <filesystem>
#include <map>
#include <string>
#include <vector>

namespace fs = std::filesystem;

struct Icon {
  std::string name;        // Filename based (Default)
  std::string customName;  // User-assigned name
  std::string path;
  std::vector<std::string> tags;
  Texture2D thumbnail = {0}; // Loaded on demand
};

struct IconCategory {
  std::string name;
  std::vector<Icon> icons;
  bool isLoaded = false; // Have we loaded the textures yet?
};

// --- FONTS ---
enum class LabelFontType { System, User };

struct FontAsset {
  std::string name;
  std::string path;
  LabelFontType type;
  Font font = {0};
  bool isLoaded = false;
};

// --- MANAGER ---
class AssetManager {
public:
  static AssetManager &Get() {
    static AssetManager instance;
    return instance;
  }

  // Scans assets/icons and builds the category list
  void RefreshLibrary(const std::string &providedBasePath);
  void RefreshBorders(const std::string &providedBasePath);
  void RefreshFonts(const std::vector<std::string> &providedUserPaths,
                    const std::vector<std::string> &providedSystemPaths);

  void AddFontsToList(std::vector<std::string> &ScanPaths,
                      std::vector<FontAsset> &targetList, LabelFontType type);

  // Returns the list of categories found
  std::vector<IconCategory> &GetCategories() { return categories; }
  std::vector<IconCategory> &GetBorders() { return borderCategories; }

  const std::vector<FontAsset> &GetFontList() { return fonts; }

  // Loads textures for a specific category (Lazy Loading)
  void LoadCategoryTextures(int categoryIndex);
  void LoadBorderTextures(int categoryIndex);
  Font GetFont(const std::string &name);
  Font GetDefaultFont() { return defaultFont; }

  // Metadata & Library Management
  void SaveMetadata();
  void LoadMetadata();
  bool MoveIcon(Icon &icon, const std::string &newCategory);
  void UpdateIconMetadata(const std::string &path, const std::string &newName,
                          const std::vector<std::string> &newTags);

  // Loading Pipeline
  void InitializeLoadQueue(); // Call this at startup
  bool ProcessLoadQueue(int batchSize = 5); // Returns false when done
  float GetLoadProgress(); // 0.0f to 1.0f
  std::string GetCurrentLoadItem(); // e.g. "Loading Office/printer.png..."

  // Actions
  bool ImportFont(const std::string &sourcePath);
  int ImportUserIcons(const std::vector<std::string> &sourcePaths);

#ifdef UNIT_TESTING
  // --- Test Path Configuration (for Unit Tests) ---
  void SetTestFontPaths(const std::vector<std::string> &userPaths,
                        const std::vector<std::string> &systemPaths) {
    testUserFontPaths = userPaths;
    testSystemFontPaths = systemPaths;
  }

  const std::vector<FontAsset> &GetUserFontList() { return userFonts; }
  const std::vector<FontAsset> &GetSystemFontList() { return systemFonts; }

  void SetTestIconPath(const std::string &path) { testIconsBasePath = path; }

  void SetTestBorderPath(const std::string &path) {
    testBordersBasePath = path;
  }

#endif

private:
  AssetManager() {
    RefreshLibrary("");
    RefreshBorders("assets/borders");
    RefreshFonts({}, {}); // System fonts will be added in RefreshFonts
  }
  std::vector<IconCategory> categories;
  std::vector<IconCategory> borderCategories;
  Font defaultFont = {0};
  std::vector<FontAsset>
      fonts; // Combined list of all fonts (system + user) for easy access
  std::vector<FontAsset>
      systemFonts; // Separate list for system fonts to avoid confusion
  std::vector<FontAsset>
      userFonts; // Separate list for user fonts to avoid confusion

  // --- Loading State ---
  std::vector<Icon *> loadQueue;
  size_t totalToLoad = 0;
  size_t currentLoadIndex = 0;

// --- Test Paths (always declared, only setters are conditional) ---
#ifdef UNIT_TESTING
  std::vector<std::string> testUserFontPaths;
  std::vector<std::string> testSystemFontPaths;
  std::string testIconsBasePath;
  std::string testBordersBasePath;
#endif
};
