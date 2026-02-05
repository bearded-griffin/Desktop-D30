#include "utils.h"
#include "types.h"
#include <gtest/gtest.h>
#include <fstream>
#include <cstdio>

class SettingsTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Use a fixed filename for the test settings file
        test_settings_filename = "test_settings.json";
    }

    void TearDown() override {
        // Clean up the temporary file
        std::remove(test_settings_filename.c_str());
    }

    std::string test_settings_filename;
};

TEST_F(SettingsTest, SaveAndLoadSettings) {
    // 1. Create AppSettings with some settings
    AppSettings settingsToSave;
    settingsToSave.darkTheme = true;
    settingsToSave.showGrid = false;

    // 2. Save the settings
    Utils::SaveSettings(settingsToSave, test_settings_filename);

    // 3. Create new AppSettings and load the settings
    AppSettings settingsToLoad;
    Utils::LoadSettings(settingsToLoad, test_settings_filename);

    // 4. Check if the settings are loaded correctly
    EXPECT_TRUE(settingsToLoad.darkTheme);
    EXPECT_FALSE(settingsToLoad.showGrid);
}

TEST_F(SettingsTest, LoadSettings_NonexistentFile) {
    // Ensure it doesn't crash and uses default values
    AppSettings settings;
    // remove the file to make sure it doesn't exist
    std::remove(test_settings_filename.c_str());
    ASSERT_NO_THROW(Utils::LoadSettings(settings, test_settings_filename));
    EXPECT_FALSE(settings.darkTheme); // Default in AppSettings is false
    EXPECT_TRUE(settings.showGrid);   // Default in AppSettings is true
}

TEST_F(SettingsTest, LoadSettings_CorruptedFile) {
    std::ofstream tmp_file(test_settings_filename);
    tmp_file << "{ \"darkTheme\": true, \"showGrid\": "; // Malformed JSON
    tmp_file.close();

    AppSettings settings;
    ASSERT_NO_THROW(Utils::LoadSettings(settings, test_settings_filename));
    // Should retain default values
    EXPECT_FALSE(settings.darkTheme); // Default in AppSettings is false
    EXPECT_TRUE(settings.showGrid);   // Default in AppSettings is true
}
