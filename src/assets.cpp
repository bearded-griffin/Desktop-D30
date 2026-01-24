/*!***************************************************
 * @file     assets.cpp
 * @brief    Supporting functions for icons
 * @details  Creates the Icon Library from the file
 * system.
 * @note     
 * @date     2026.01.23
 * @author   bearded.griffin
 ****************************************************/

#include "assets.h"
#include <algorithm>
#include <iostream>


/*!***************************************************
 * @brief    Rebuilds the icon library
 * @details  Reads the file system to see if there are
 * any icons that are available for use.
 * @param    
 * @return   void
 * @note     
 * @date     2026.01.23
 * @author   bearded.griffin
 ****************************************************/
void AssetManager::RefreshLibrary() {
  categories.clear();

  // Path to assets/icons
  // Note: When running from build/, we might need to go up one level or copy
  // assets For now, assume "assets/icons" exists next to the binary
  std::string basePath = "assets/icons";

  if (!fs::exists(basePath)) {
    std::cerr << "[Assets] Warning: 'assets/icons' folder not found!"
              << std::endl;
    // Try creating it to be helpful
    fs::create_directories(basePath);
    return;
  }

  for (const auto &entry : fs::directory_iterator(basePath)) {
    if (entry.is_directory()) {
      IconCategory cat;
      cat.name = entry.path().filename().string();

      // Scan files inside this category
      for (const auto &file : fs::directory_iterator(entry.path())) {
        if (file.path().extension() == ".png" ||
            file.path().extension() == ".jpg") {
          Icon icon;
          icon.name = file.path().stem().string(); // "skull" from "skull.png"
          icon.path = file.path().string();
          cat.icons.push_back(icon);
        }
      }

      // Only add if it has icons
      if (!cat.icons.empty()) {
        categories.push_back(cat);
      }
    }
  }

  // Sort categories alphabetically
  std::sort(categories.begin(), categories.end(),
            [](const IconCategory &a, const IconCategory &b) {
              return a.name < b.name;
            });
}

/*!***************************************************
 * @brief    Loads Thumbnail of icon
 * @details  Creates the icons thumbnail as a preview
 * in the Icon Library.
 * @param    categoryIndex int 
 * @return   void 
 * @note     
 * @date     2026.01.23
 * @author   bearded.griffin
 ****************************************************/
void AssetManager::LoadCategoryTextures(int categoryIndex) {
  if (categoryIndex < 0 || categoryIndex >= categories.size())
    return;

  IconCategory &cat = categories[categoryIndex];
  if (cat.isLoaded)
    return; // Already done

  for (auto &icon : cat.icons) {
    if (icon.thumbnail.id == 0) {
      Image img = LoadImage(icon.path.c_str());
      // Resize for UI thumbnail if too huge
      if (img.width > 64 || img.height > 64) {
        ImageResize(&img, 64, 64);
      }
      icon.thumbnail = LoadTextureFromImage(img);
      UnloadImage(img);
    }
  }
  cat.isLoaded = true;
}