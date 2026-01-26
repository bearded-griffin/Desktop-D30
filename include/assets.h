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

// --- FONTS ---
enum class LabelFontType { System, User };

struct FontAsset {
    std::string name;
    std::string path;
    LabelFontType type;
    Font font = { 0 };
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
  void RefreshFonts();

  // Returns the list of categories found
  std::vector<IconCategory> &GetCategories() { return categories; }
  const std::vector<FontAsset>& GetFontList() { return fonts; }

  // Loads textures for a specific category (Lazy Loading)
  void LoadCategoryTextures(int categoryIndex);
  Font GetFont(const std::string& name);

  // Actions
  bool ImportFont(const std::string& sourcePath);

private:
  AssetManager() { 
    RefreshLibrary();
    RefreshFonts(); 
  }
  std::vector<IconCategory> categories;
  std::vector<FontAsset> fonts;
};
