/*!***************************************************
 * @file     assets.h
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

class AssetManager {
public:
  static AssetManager &Get() {
    static AssetManager instance;
    return instance;
  }

  // Scans assets/icons and builds the category list
  void RefreshLibrary();

  // Returns the list of categories found
  std::vector<IconCategory> &GetCategories() { return categories; }

  // Loads textures for a specific category (Lazy Loading)
  void LoadCategoryTextures(int categoryIndex);

private:
  AssetManager() { RefreshLibrary(); }
  std::vector<IconCategory> categories;
};