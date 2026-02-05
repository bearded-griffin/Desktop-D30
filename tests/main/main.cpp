#include <gtest/gtest.h>
#include "raylib.h"
#include "assets.h"

int main(int argc, char **argv) {
    // Initialize a headless window for testing
    InitWindow(1, 1, "Test");

    // Initialize asset manager to load fonts
    AssetManager::Get().RefreshFonts();

    ::testing::InitGoogleTest(&argc, argv);
    int result = RUN_ALL_TESTS();

    CloseWindow();
    return result;
}
