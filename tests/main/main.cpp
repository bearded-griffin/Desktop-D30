#include "assets.h"
#include "raylib.h"
#include "rlImGui.h"
#include <gtest/gtest.h>

int main(int argc, char **argv) {
  // Initialize a headless window for testing
  InitWindow(1, 1, "Test");
  rlImGuiSetup(true);

  // Initialize asset manager to load fonts
  AssetManager::Get().RefreshFonts({}, {});

  ::testing::InitGoogleTest(&argc, argv);
  int result = RUN_ALL_TESTS();

  rlImGuiShutdown();
  CloseWindow();
  return result;
}
