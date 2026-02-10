#include "assets.h"
#include <filesystem>
#include <fstream> // For creating dummy files
#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace fs = std::filesystem;

// --- Test Fixture ---
class AssetManagerTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Create a temporary directory for tests
    tempDir = fs::temp_directory_path() / "AssetManagerTest";
    fs::create_directories(tempDir);

    // Setup dummy asset directories for testing AssetManager's scanning logic
    userFontsDir = tempDir / "assets" / "fonts";
    fs::create_directories(userFontsDir);

    systemFontsDir = tempDir / "system_fonts"; // Mimic a system font dir
    fs::create_directories(systemFontsDir);

    iconsBaseDir = tempDir / "assets" / "icons";
    fs::create_directories(iconsBaseDir / "Category1");
    fs::create_directories(iconsBaseDir / "Category2");

    bordersBaseDir = tempDir / "assets" / "borders";
    fs::create_directories(bordersBaseDir / "BorderCategory1");

    // AssetManager relies on current working directory
    originalCwd = fs::current_path();
    fs::current_path(tempDir);

    // Set test paths for AssetManager
    AssetManager::Get().SetTestFontPaths({userFontsDir.string()},
                                         {systemFontsDir.string()});
    AssetManager::Get().SetTestIconPath(iconsBaseDir.string());
    AssetManager::Get().SetTestBorderPath(bordersBaseDir.string());

    // Clear any previous AssetManager state (singleton) and rescan with test
    // paths Refresh methods effectively clear and re-scan.
    AssetManager::Get().RefreshFonts({}, {});
    AssetManager::Get().RefreshLibrary(iconsBaseDir.string());
    AssetManager::Get().RefreshBorders(bordersBaseDir.string());
  }

  void TearDown() override {
    fs::current_path(originalCwd); // Restore original CWD
    fs::remove_all(tempDir);       // Clean up temporary directory
  }

  fs::path tempDir;
  fs::path userFontsDir;
  fs::path systemFontsDir;
  fs::path iconsBaseDir;
  fs::path bordersBaseDir;
  fs::path originalCwd;
};

// --- Tests for RefreshFonts ---

// This test requires mocking of getenv and directory iteration for system
// paths, which is more complex due to platform specifics and
// recursive_directory_iterator. For now, we'll focus on user fonts and explicit
// paths. TEST_F(AssetManagerTest, RefreshFontsScansSystemFonts) { /* ... */ }

TEST_F(AssetManagerTest, RefreshFontsHandlesEmptyDirectory) {
  // userFontsDir is empty by default after setup
  AssetManager::Get().RefreshFonts({}, {});
  const auto &fontList = AssetManager::Get().GetUserFontList();
  EXPECT_TRUE(fontList.empty());
}

TEST_F(AssetManagerTest, RefreshFontsScansUserFonts) {
  // Create dummy font files in userFontsDir
  std::ofstream(userFontsDir / "userfont1.ttf", std::ios::out).close();
  std::ofstream(userFontsDir / "userfont2.otf", std::ios::out).close();
  std::ofstream(userFontsDir / "notafont.txt", std::ios::out)
      .close(); // Should be ignored

  AssetManager::Get().RefreshFonts({}, {});
  const auto &fontList = AssetManager::Get().GetUserFontList();

  ASSERT_EQ(fontList.size(), 2);
  EXPECT_EQ(fontList[0].name, "Userfont1");
  EXPECT_EQ(fontList[0].type, LabelFontType::User);
  EXPECT_EQ(fontList[1].name, "Userfont2");
  EXPECT_EQ(fontList[1].type, LabelFontType::User);
}

TEST_F(AssetManagerTest, RefreshFontsIgnoresNonFontFiles) {
  std::ofstream(userFontsDir / "image.png").close();
  std::ofstream(userFontsDir / "document.pdf").close();

  AssetManager::Get().RefreshFonts({}, {});
  const auto &fontList = AssetManager::Get().GetFontList();
  EXPECT_TRUE(fontList.empty());
}

// --- Tests for RefreshLibrary (Icons) ---
TEST_F(AssetManagerTest, RefreshLibraryScansIconCategories) {
  std::ofstream(iconsBaseDir / "Category1" / "icon1.png").close();
  std::ofstream(iconsBaseDir / "Category1" / "icon2.jpg").close();
  std::ofstream(iconsBaseDir / "Category2" / "icon3.png").close();
  std::ofstream(iconsBaseDir / "Category2" / "ignore.txt").close();

  AssetManager::Get().RefreshLibrary(iconsBaseDir.string());
  const auto &categories = AssetManager::Get().GetCategories();

  ASSERT_EQ(categories.size(), 2); // Category1, Category2
  EXPECT_EQ(categories[0].name, "Category1");
  EXPECT_EQ(categories[1].name, "Category2");

  ASSERT_EQ(categories[0].icons.size(), 2);
  EXPECT_EQ(categories[0].icons[0].name, "Icon1");
  EXPECT_EQ(categories[0].icons[1].name, "Icon2");

  ASSERT_EQ(categories[1].icons.size(), 1);
  EXPECT_EQ(categories[1].icons[0].name, "Icon3");
}

TEST_F(AssetManagerTest, RefreshLibraryHandlesEmptyCategories) {
  fs::create_directories(iconsBaseDir /
                         "EmptyCategory"); // Create an empty category

  AssetManager::Get().RefreshLibrary(iconsBaseDir.string());
  const auto &categories = AssetManager::Get().GetCategories();

  // Should not add empty categories
  EXPECT_TRUE(categories.empty());
}

// --- Tests for RefreshBorders ---
TEST_F(AssetManagerTest, RefreshBordersScansBorderCategories) {
  std::ofstream(bordersBaseDir / "BorderCategory1" / "border1.png").close();
  std::ofstream(bordersBaseDir / "BorderCategory1" / "border2.jpeg").close();

  AssetManager::Get().RefreshBorders(bordersBaseDir.string());
  const auto &categories = AssetManager::Get().GetBorders();

  ASSERT_EQ(categories.size(), 1);
  EXPECT_EQ(categories[0].name, "BorderCategory1");
  ASSERT_EQ(categories[0].icons.size(), 2);
  EXPECT_EQ(categories[0].icons[0].name, "border1");
  EXPECT_EQ(categories[0].icons[1].name, "border2");
}

// --- Tests for ImportFont ---
TEST_F(AssetManagerTest, ImportFontSuccessfullyCopiesFile) {
  // Create a dummy font file outside the userFontsDir
  fs::path sourceFontPath = tempDir / "sourcefont.ttf";
  std::ofstream(sourceFontPath).close();

  EXPECT_TRUE(AssetManager::Get().ImportFont(sourceFontPath.string()));

  // Check if it's copied to userFontsDir
  fs::path destFontPath = userFontsDir / "sourcefont.ttf";
  EXPECT_TRUE(fs::exists(destFontPath));

  // Check if RefreshFonts was called (implicitly through getting fontList)
  const auto &fontList = AssetManager::Get().GetFontList();
  ASSERT_EQ(fontList.size(), 1);
  EXPECT_EQ(fontList[0].name, "Sourcefont");
}

TEST_F(AssetManagerTest, ImportFontHandlesNonExistentSource) {
  fs::path nonExistentPath = tempDir / "nonexistent.ttf";
  EXPECT_FALSE(AssetManager::Get().ImportFont(nonExistentPath.string()));
}

// Add more tests for error handling, edge cases, etc.