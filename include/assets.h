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
  std::string name;
  std::string path;
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
  void RefreshLibrary();
  void RefreshBorders();
  void RefreshFonts();

  // Returns the list of categories found
  std::vector<IconCategory> &GetCategories() { return categories; }
  std::vector<IconCategory> &GetBorders() { return borderCategories; }
  const std::vector<FontAsset> &GetFontList() { return fonts; }

  // Loads textures for a specific category (Lazy Loading)
  void LoadCategoryTextures(int categoryIndex);
  void LoadBorderTextures(int categoryIndex);
  Font GetFont(const std::string &name);

  // Actions
  bool ImportFont(const std::string &sourcePath);

private:
  AssetManager() {
    RefreshLibrary();
    RefreshBorders();
    RefreshFonts();
  }
  std::vector<IconCategory> categories;
  std::vector<IconCategory> borderCategories;
  std::vector<FontAsset> fonts;
};
