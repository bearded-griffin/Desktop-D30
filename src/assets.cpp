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
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

namespace {
std::string FormatDisplayName(std::string name) {
  std::replace(name.begin(), name.end(), '_', ' ');
  std::replace(name.begin(), name.end(), '-', ' ');
  if (!name.empty())
    name[0] = std::toupper(name[0]);
  return name;
}
} // namespace

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

  std::vector<std::string> searchPaths;
  if (!providedBasePath.empty()) {
    searchPaths.push_back(providedBasePath);
  } else {
    searchPaths.push_back("assets/icons/built-in");
    searchPaths.push_back("assets/icons/user");
  }

  for (const auto &basePath : searchPaths) {
    if (!fs::exists(basePath)) {
      fs::create_directories(basePath);
      continue;
    }

    for (const auto &entry : fs::recursive_directory_iterator(basePath)) {
      if (fs::is_regular_file(entry)) {
        std::string ext = entry.path().extension().string();
        if (ext == ".png" || ext == ".jpg" || ext == ".jpeg") {
          std::string categoryName =
              entry.path().parent_path().filename().string();

          // If the file is in the root of built-in or user, call it "General"
          if (categoryName == "built-in" || categoryName == "user" ||
              categoryName == "icons") {
            categoryName = "General";
          }

          // Find or create category
          IconCategory *targetCat = nullptr;
          for (auto &cat : categories) {
            if (cat.name == categoryName) {
              targetCat = &cat;
              break;
            }
          }

          if (!targetCat) {
            categories.push_back({categoryName, {}, false});
            targetCat = &categories.back();
          }

          Icon icon;
          icon.name = FormatDisplayName(entry.path().stem().string());
          icon.path = entry.path().string();
          icon.type = AssetType::Icon;
          targetCat->icons.push_back(icon);
        }
      }
    }
  }

  // Sort icons within categories
  for (auto &cat : categories) {
    std::sort(cat.icons.begin(), cat.icons.end(),
              [](const Icon &a, const Icon &b) { return a.name < b.name; });
  }

  // Sort categories alphabetically
  std::sort(categories.begin(), categories.end(),
            [](const IconCategory &a, const IconCategory &b) {
              return a.name < b.name;
            });

  LoadMetadata(); // Overlay metadata after scanning files
}

/*!***************************************************
 * @brief    Saves icon metadata to JSON
 * @details  Stores tags and custom names for icons.
 * @return   void
 ****************************************************/
void AssetManager::SaveMetadata() {
  auto SaveTo = [](const std::vector<IconCategory> &cats,
                   const std::string &filePath) {
    nlohmann::json j;
    for (const auto &cat : cats) {
      for (const auto &icon : cat.icons) {
        if (!icon.tags.empty() || !icon.customName.empty()) {
          nlohmann::json meta;
          if (!icon.customName.empty())
            meta["name"] = icon.customName;
          if (!icon.tags.empty())
            meta["tags"] = icon.tags;
          j[icon.path] = meta;
        }
      }
    }
    std::ofstream file(filePath);
    if (file.is_open()) {
      file << j.dump(4);
    }
  };

  SaveTo(categories, "assets/icons/metadata.json");
  SaveTo(borderCategories, "assets/borders/metadata.json");
}

/*!***************************************************
 * @brief    Loads icon metadata from JSON
 * @details  Overlays custom names and tags onto icons.
 * @return   void
 ****************************************************/
void AssetManager::LoadMetadata() {
  auto LoadFrom = [](std::vector<IconCategory> &cats,
                     const std::string &filePath) {
    std::ifstream file(filePath);
    if (!file.is_open())
      return;

    try {
      nlohmann::json j;
      file >> j;

      for (auto &cat : cats) {
        for (auto &icon : cat.icons) {
          if (j.contains(icon.path)) {
            auto meta = j[icon.path];
            if (meta.contains("name"))
              icon.customName = meta["name"].get<std::string>();
            if (meta.contains("tags"))
              icon.tags = meta["tags"].get<std::vector<std::string>>();
          }
        }
      }
    } catch (...) {
      std::cerr << "[Assets] Error: Failed to parse " << filePath << std::endl;
    }
  };

  LoadFrom(categories, "assets/icons/metadata.json");
  LoadFrom(borderCategories, "assets/borders/metadata.json");
}

/*!***************************************************
 * @brief    Moves an icon to a new category
 * @details  Physically moves the file and updates metadata.
 * @param    icon Icon&
 * @param    newCategory const std::string&
 * @return   bool
 ****************************************************/
bool AssetManager::MoveAsset(Icon &asset, const std::string &newCategory) {
  fs::path oldPath(asset.path);
  std::string filename = oldPath.filename().string();

  // Determine root
  std::string root;
  if (asset.type == AssetType::Icon) {
    root = "assets/icons/user";
    if (asset.path.find("assets/icons/built-in") != std::string::npos) {
      root = "assets/icons/built-in";
    }
  } else {
    root = "assets/borders";
  }

  fs::path newDirPath = fs::path(root) / newCategory;
  if (!fs::exists(newDirPath)) {
    fs::create_directories(newDirPath);
  }

  fs::path newPath = newDirPath / filename;

  try {
    fs::rename(oldPath, newPath);
    asset.path = newPath.string();
    SaveMetadata();
    if (asset.type == AssetType::Icon)
      RefreshLibrary("");
    else
      RefreshBorders("");
    return true;
  } catch (const std::exception &e) {
    std::cerr << "[Assets] Move failed: " << e.what() << std::endl;
    return false;
  }
}

void AssetManager::UpdateAssetMetadata(
    Icon &asset, const std::string &newName,
    const std::vector<std::string> &newTags) {
  asset.customName = newName;
  asset.tags = newTags;
  SaveMetadata();
}

/*!***************************************************
 * @brief    Prepares the load queue
 * @details  Flattens all icons into a single list
 * so we can load them incrementally.
 * @return   void
 ****************************************************/
void AssetManager::InitializeLoadQueue() {
  loadQueue.clear();
  for (auto &cat : categories) {
    for (auto &icon : cat.icons) {
      if (icon.thumbnail.id == 0) { // Only queue unloaded ones
        loadQueue.push_back(&icon);
      }
    }
  }
  for (auto &cat : borderCategories) {
    for (auto &icon : cat.icons) {
      if (icon.thumbnail.id == 0) {
        loadQueue.push_back(&icon);
      }
    }
  }
  totalToLoad = loadQueue.size();
  currentLoadIndex = 0;
}

/*!***************************************************
 * @brief    Loads a batch of icons
 * @details  Loads 'batchSize' textures then returns.
 * @param    batchSize int
 * @return   bool True if there is more to load
 ****************************************************/
bool AssetManager::ProcessLoadQueue(int batchSize) {
  if (currentLoadIndex >= totalToLoad)
    return false;

  for (int i = 0; i < batchSize; i++) {
    if (currentLoadIndex >= totalToLoad)
      break;

    Icon *icon = loadQueue[currentLoadIndex];
    if (icon && icon->thumbnail.id == 0) {
      Image img = LoadImage(icon->path.c_str());
      if (img.width > 64 || img.height > 64) {
        ImageResize(&img, 64, 64);
      }
      icon->thumbnail = LoadTextureFromImage(img);
      UnloadImage(img);
    }
    currentLoadIndex++;
  }
  return currentLoadIndex < totalToLoad;
}

float AssetManager::GetLoadProgress() {
  if (totalToLoad == 0)
    return 1.0f;
  return (float)currentLoadIndex / (float)totalToLoad;
}

std::string AssetManager::GetCurrentLoadItem() {
  if (currentLoadIndex < totalToLoad) {
    fs::path p(loadQueue[currentLoadIndex]->path);
    return "Loading " + p.filename().string();
  }
  return "Ready!";
}

void AssetManager::UnloadAssets() {
  // Unload Icon Textures
  for (auto &cat : categories) {
    for (auto &icon : cat.icons) {
      if (icon.thumbnail.id != 0) {
        UnloadTexture(icon.thumbnail);
        icon.thumbnail = {0};
      }
    }
    cat.isLoaded = false;
  }

  // Unload Border Textures
  for (auto &cat : borderCategories) {
    for (auto &icon : cat.icons) {
      if (icon.thumbnail.id != 0) {
        UnloadTexture(icon.thumbnail);
        icon.thumbnail = {0};
      }
    }
    cat.isLoaded = false;
  }

  // Unload Font Textures
  for (auto &f : fonts) {
    if (f.isLoaded) {
      UnloadFont(f.font);
      f.font = {0};
      f.isLoaded = false;
    }
  }

  if (defaultFont.texture.id != 0) {
    UnloadFont(defaultFont);
    defaultFont = {0};
  }
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

  // Load a high-quality default font if not already loaded
  if (defaultFont.texture.id == 0) {
    const char *fallbackPaths[] = {
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/TTF/DejaVuSans.ttf",
        "/usr/share/fonts/liberation/LiberationSans-Regular.ttf",
        "C:\\Windows\\Fonts\\arial.ttf" // Windows fallback
    };

    for (const char *path : fallbackPaths) {
      if (fs::exists(path)) {
        defaultFont = LoadFontEx(path, 96, 0, 0);
        GenTextureMipmaps(&defaultFont.texture);
        SetTextureFilter(defaultFont.texture, TEXTURE_FILTER_BILINEAR);
        break;
      }
    }

    // If still not loaded, fall back to raylib default
    if (defaultFont.texture.id == 0) {
      defaultFont = GetFontDefault();
    }
  }

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
      if (!homeEnv) homeEnv = std::getenv("USERPROFILE");

      if (homeEnv) {
        systemScanPaths.push_back("/usr/share/fonts");
        systemScanPaths.push_back("/usr/local/share/fonts");
        systemScanPaths.push_back(std::string(homeEnv) + "/.local/share/fonts");
        systemScanPaths.push_back(std::string(homeEnv) + "/.fonts");
        systemScanPaths.push_back("C:\\Windows\\Fonts");
        systemScanPaths.push_back(std::string(homeEnv) + "\\AppData\\Local\\Microsoft\\Windows\\Fonts");
      }
    }
#else
    const char *homeEnv = std::getenv("HOME");
    if (!homeEnv) homeEnv = std::getenv("USERPROFILE");

    if (homeEnv) {
      systemScanPaths.push_back("/usr/share/fonts");
      systemScanPaths.push_back("/usr/local/share/fonts");
      systemScanPaths.push_back(std::string(homeEnv) + "/.local/share/fonts");
      systemScanPaths.push_back(std::string(homeEnv) + "/.fonts");
      systemScanPaths.push_back("C:\\Windows\\Fonts");
      systemScanPaths.push_back(std::string(homeEnv) + "\\AppData\\Local\\Microsoft\\Windows\\Fonts");
    }
#endif
  }

  AddFontsToList(systemScanPaths, systemFonts, LabelFontType::System);
  AddFontsToList(userScanPaths, userFonts, LabelFontType::User);

  // Combine user and system fonts into a single list for easy access, with user
  // fonts first

  // Sort user fonts alphabetically
  std::sort(
      userFonts.begin(), userFonts.end(),
      [](const FontAsset &a, const FontAsset &b) { return a.name < b.name; });

  // Sort system fonts alphabetically
  std::sort(
      systemFonts.begin(), systemFonts.end(),
      [](const FontAsset &a, const FontAsset &b) { return a.name < b.name; });

  fonts.insert(fonts.end(), userFonts.begin(), userFonts.end());
  fonts.insert(fonts.end(), systemFonts.begin(), systemFonts.end());

  // Re-build fast lookup map
  fontMap.clear();
  for (auto &f : fonts) {
    fontMap[f.name] = &f;
  }
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
          f.name = FormatDisplayName(entry.path().stem().string());
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
    return defaultFont;

  auto it = fontMap.find(name);
  if (it != fontMap.end()) {
    FontAsset *f = it->second;
    if (!f->isLoaded) {
      f->font = LoadFontEx(f->path.c_str(), 96, 0, 0); // Load Big for quality
      GenTextureMipmaps(&f->font.texture);
      SetTextureFilter(f->font.texture, TEXTURE_FILTER_BILINEAR);
      f->isLoaded = true;
    }
    return f->font;
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
 * @brief    Imports user icons
 * @details  Copies selected files to assets/icons/user/Imports
 * @param    sourcePaths const std::vector<std::string>&
 * @return   int Number of files successfully imported
 ****************************************************/
int AssetManager::ImportUserIcons(const std::vector<std::string> &sourcePaths) {
  if (sourcePaths.empty())
    return 0;

  std::string destDir = "assets/icons/user/Imports";
  if (!fs::exists(destDir)) {
    fs::create_directories(destDir);
  }

  int successCount = 0;
  for (const auto &src : sourcePaths) {
    if (!fs::exists(src))
      continue;

    fs::path srcPath(src);
    std::string filename = srcPath.filename().string();
    fs::path destPath = fs::path(destDir) / filename;

    // Handle duplicate names by appending a counter
    int counter = 1;
    while (fs::exists(destPath)) {
      std::string stem = srcPath.stem().string();
      std::string ext = srcPath.extension().string();
      destPath =
          fs::path(destDir) / (stem + "_" + std::to_string(counter++) + ext);
    }

    try {
      fs::copy_file(srcPath, destPath);
      successCount++;
    } catch (const std::exception &e) {
      std::cerr << "[Assets] Failed to import icon " << filename << ": "
                << e.what() << std::endl;
    }
  }

  if (successCount > 0) {
    RefreshLibrary(""); // Refresh to show new files
  }
  return successCount;
}

/*!***************************************************
 * @brief    Imports user borders
 * @details  Copies selected files to assets/borders/Imports
 * @param    sourcePaths const std::vector<std::string>&
 * @return   int Number of files successfully imported
 ****************************************************/
int AssetManager::ImportUserBorders(
    const std::vector<std::string> &sourcePaths) {
  if (sourcePaths.empty())
    return 0;

  std::string destDir = "assets/borders/Imports";
  if (!fs::exists(destDir)) {
    fs::create_directories(destDir);
  }

  int successCount = 0;
  for (const auto &src : sourcePaths) {
    if (!fs::exists(src))
      continue;

    fs::path srcPath(src);
    std::string filename = srcPath.filename().string();
    fs::path destPath = fs::path(destDir) / filename;

    int counter = 1;
    while (fs::exists(destPath)) {
      std::string stem = srcPath.stem().string();
      std::string ext = srcPath.extension().string();
      destPath =
          fs::path(destDir) / (stem + "_" + std::to_string(counter++) + ext);
    }

    try {
      fs::copy_file(srcPath, destPath);
      successCount++;
    } catch (const std::exception &e) {
      std::cerr << "[Assets] Failed to import border " << filename << ": "
                << e.what() << std::endl;
    }
  }

  if (successCount > 0) {
    RefreshBorders("");
  }
  return successCount;
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

  for (const auto &entry : fs::recursive_directory_iterator(basePath)) {
    if (fs::is_regular_file(entry)) {
      std::string ext = entry.path().extension().string();
      if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp") {
        std::string categoryName =
            entry.path().parent_path().filename().string();

        if (categoryName == "borders") {
          categoryName = "General";
        }

        // Find or create category
        IconCategory *targetCat = nullptr;
        for (auto &cat : borderCategories) {
          if (cat.name == categoryName) {
            targetCat = &cat;
            break;
          }
        }

        if (!targetCat) {
          borderCategories.push_back({categoryName, {}, false});
          targetCat = &borderCategories.back();
        }

        Icon icon;
        icon.name = FormatDisplayName(entry.path().stem().string());
        icon.path = entry.path().string();
        icon.type = AssetType::Border;
        targetCat->icons.push_back(icon);
      }
    }
  }

  // Sort icons within categories
  for (auto &cat : borderCategories) {
    std::sort(cat.icons.begin(), cat.icons.end(),
              [](const Icon &a, const Icon &b) { return a.name < b.name; });
  }

  // Sort categories alphabetically
  std::sort(borderCategories.begin(), borderCategories.end(),
            [](const IconCategory &a, const IconCategory &b) {
              return a.name < b.name;
            });

  LoadMetadata();
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