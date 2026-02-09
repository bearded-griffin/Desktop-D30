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
#include <cstdlib>
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
void AssetManager::RefreshLibrary(const std::string &providedBasePath) {
  categories.clear();

  std::string basePath =
      providedBasePath.empty() ? "assets/icons" : providedBasePath;

  if (!fs::exists(basePath)) {
    std::cerr << "[Assets] Warning: '" << basePath << "' folder not found!"
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
        // Sort icons alphabetically within the category
        std::sort(cat.icons.begin(), cat.icons.end(),
                  [](const Icon &a, const Icon &b) { return a.name < b.name; });
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

/*!***************************************************
 * @brief    Reloads available fonts
 * @details  It refreshes the fonts that are availible
 * from the System and from the assets/fonts folder.
 * @return   void
 * @note
 * @date     2026.01.26
 * @author   bearded.griffin
 ****************************************************/
void AssetManager::RefreshFonts(
    const std::vector<std::string> &providedUserPaths,
    const std::vector<std::string> &providedSystemPaths) {
  userFonts.clear();
  systemFonts.clear();
  fonts.clear();

  // Determine paths to scan
  std::vector<std::string> userScanPaths;
  std::vector<std::string> systemScanPaths;

  if (!providedUserPaths.empty()) {
    userScanPaths = providedUserPaths;
  } else {
#ifdef UNIT_TESTING
    if (!testUserFontPaths.empty()) {
      userScanPaths = testUserFontPaths;
    } else {
      userScanPaths.push_back("assets/fonts");
    }
#else
    userScanPaths.push_back("assets/fonts");
#endif
  }

  if (!providedSystemPaths.empty()) {
    systemScanPaths = providedSystemPaths;
  } else {
#ifdef UNIT_TESTING
    if (!testSystemFontPaths.empty()) {
      systemScanPaths = testSystemFontPaths;
    } else {
      const char *homeEnv = std::getenv("HOME");
      if (homeEnv) {
        systemScanPaths.push_back("/usr/share/fonts");
        systemScanPaths.push_back("/usr/local/share/fonts");
        systemScanPaths.push_back(std::string(homeEnv) + "/.local/share/fonts");
        systemScanPaths.push_back(std::string(homeEnv) + "/.fonts");
      }
    }
#else
    const char *homeEnv = std::getenv("HOME");
    if (homeEnv) {
      systemScanPaths.push_back("/usr/share/fonts");
      systemScanPaths.push_back("/usr/local/share/fonts");
      systemScanPaths.push_back(std::string(homeEnv) + "/.local/share/fonts");
      systemScanPaths.push_back(std::string(homeEnv) + "/.fonts");
    }
#endif
  }

  AddFontsToList(systemScanPaths, systemFonts, LabelFontType::System);
  AddFontsToList(userScanPaths, userFonts, LabelFontType::User);

  // Combine user and system fonts into a single list for easy access, with user
  // fonts first

  // Sort user fonts alphabetically
  std::sort(userFonts.begin(), userFonts.end(),
            [](const FontAsset &a, const FontAsset &b) {
              return a.name < b.name;
            });

  // Sort system fonts alphabetically
  std::sort(systemFonts.begin(), systemFonts.end(),
            [](const FontAsset &a, const FontAsset &b) {
              return a.name < b.name;
            });

  fonts.insert(fonts.end(), userFonts.begin(), userFonts.end());
  fonts.insert(fonts.end(), systemFonts.begin(), systemFonts.end());
}

void AssetManager::AddFontsToList(std::vector<std::string> &ScanPaths,
                                  std::vector<FontAsset> &targetList,
                                  LabelFontType type) {
  for (const auto &basePath : ScanPaths) {
    if (!fs::exists(basePath))
      continue;
    try {
      for (const auto &entry : fs::recursive_directory_iterator(basePath)) {
        if (fs::is_directory(entry))
          continue;
        std::string ext = entry.path().extension().string();
        for (auto &c : ext)
          c = tolower(c);

        if (ext == ".ttf" || ext == ".otf") {
          FontAsset f;
          f.name = entry.path().stem().string();
          f.path = entry.path().string();
          f.type = type;

          bool exists = false;
          for (const auto &existing : targetList) {
            if (existing.name == f.name) {
              exists = true;
              break;
            }
          }
          if (!exists)
            targetList.push_back(f);
        }
      }
    } catch (...) {
    }
  }
}

/*!***************************************************
 * @brief    Loads the font
 * @details  Takes the name to find the corresponding
 * font and loads it. If the name is empty, you get the
 * default font.
 * @param    name const::string&
 * @return   Font
 * @note
 * @date     2026.01.26
 * @author   bearded.griffin
 ****************************************************/
Font AssetManager::GetFont(const std::string &name) {
  if (name.empty())
    return GetFontDefault();
  for (auto &f : fonts) {
    if (f.name == name) {
      if (!f.isLoaded) {
        f.font = LoadFontEx(f.path.c_str(), 96, 0, 0); // Load Big for quality
        GenTextureMipmaps(&f.font.texture);
        SetTextureFilter(f.font.texture, TEXTURE_FILTER_BILINEAR);
        f.isLoaded = true;
      }
      return f.font;
    }
  }
  return GetFontDefault();
}

/*!***************************************************
 * @brief    Puts font in user folder
 * @details  It takes the font and puts it in the assets/fonts
 * folder.
 * @param    sourcePath const std::string&
 * @return   bool
 * @note
 * @date     2026.01.26
 * @author   bearded.griffin
 ****************************************************/
bool AssetManager::ImportFont(const std::string &sourcePath) {
  if (!fs::exists(sourcePath))
    return false;
  std::string destDir = "assets/fonts";
  if (!fs::exists(destDir))
    fs::create_directories(destDir);
  std::string filename = fs::path(sourcePath).filename().string();
  std::string destPath = destDir + "/" + filename;
  try {
    fs::copy_file(sourcePath, destPath, fs::copy_options::overwrite_existing);
    RefreshFonts({}, {}); // Refresh to include the new font
    return true;
  } catch (...) {
    return false;
  }
}

/*!***************************************************
 * @brief    Rebuilds the border library
 * @details  Reads the file system to see if there are
 * any borders that are available for use.
 * @param
 * @return   void
 * @note
 * @date     2026.01.27
 * @author   bearded.griffin
 ****************************************************/
void AssetManager::RefreshBorders(const std::string &providedBasePath) {
  borderCategories.clear();
  std::string basePath =
      providedBasePath.empty() ? "assets/borders" : providedBasePath;
  if (!fs::exists(basePath)) {
    fs::create_directories(basePath);
    return;
  }

  for (const auto &entry : fs::directory_iterator(basePath)) {
    if (entry.is_directory()) {
      IconCategory cat;
      cat.name = entry.path().filename().string();
      for (const auto &file : fs::directory_iterator(entry.path())) {
        std::string ext = file.path().extension().string();
        if (ext == ".png" || ext == ".jpg" || ext == ".jpeg") {
          Icon icon;
          icon.name = file.path().stem().string();
          icon.path = file.path().string();
          cat.icons.push_back(icon);
        }
      }
      if (!cat.icons.empty()) {
        // Sort icons alphabetically within the category
        std::sort(cat.icons.begin(), cat.icons.end(),
                  [](const Icon &a, const Icon &b) { return a.name < b.name; });
        borderCategories.push_back(cat);
      }
    }
  }
  std::sort(borderCategories.begin(), borderCategories.end(),
            [](const IconCategory &a, const IconCategory &b) {
              return a.name < b.name;
            });
}

/*!***************************************************
 * @brief    Loads Thumbnail of border
 * @details  Reads the file system to load thumbnails for borders that are
 * available for use.
 * @param    categoryIndex int
 * @return   void
 * @note
 * @date     2026.01.27
 * @author   bearded.griffin
 ****************************************************/
void AssetManager::LoadBorderTextures(int categoryIndex) {
  if (categoryIndex < 0 || categoryIndex >= borderCategories.size())
    return;
  IconCategory &cat = borderCategories[categoryIndex];
  if (cat.isLoaded)
    return;

  for (auto &icon : cat.icons) {
    if (icon.thumbnail.id == 0) {
      Image img = LoadImage(icon.path.c_str());
      if (img.width > 64 || img.height > 64)
        ImageResize(&img, 64, 64);
      icon.thumbnail = LoadTextureFromImage(img);
      UnloadImage(img);
    }
  }
  cat.isLoaded = true;
}