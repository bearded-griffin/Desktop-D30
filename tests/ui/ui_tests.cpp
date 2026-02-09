#include "ui.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <string>

// External helper from mocks.cpp
std::string GetLastWindowTitle();

// --- Test Fixture ---
class UITest : public ::testing::Test {
protected:
  void SetUp() override {
    // Reset state before each test
    uiState = UI::UIState();
    project = Project();
  }

  UI::UIState uiState;
  Project project;
};

// --- Tests for State Management ---

TEST_F(UITest, RequestExitSetsFlag) {
  EXPECT_FALSE(uiState.exitRequested);
  UI::RequestExit(uiState);
  EXPECT_TRUE(uiState.exitRequested);
}

TEST_F(UITest, ShouldCloseReturnsForceQuit) {
  uiState.forceQuit = false;
  EXPECT_FALSE(UI::ShouldClose(uiState));

  uiState.forceQuit = true;
  EXPECT_TRUE(UI::ShouldClose(uiState));
}

TEST_F(UITest, ClearExitRequestClearsFlag) {
  uiState.exitRequested = true;
  UI::ClearExitRequest(uiState);
  EXPECT_FALSE(uiState.exitRequested);
}

// --- Tests for Window Title ---

TEST_F(UITest, UpdateWindowTitle_Default) {
  UI::UpdateWindowTitle(project);
  EXPECT_EQ(GetLastWindowTitle(), "Desktop-D30");
}

TEST_F(UITest, UpdateWindowTitle_WithFile) {
  project.projectFilePath = "/home/user/test_project.d30";
  project.isDirty = false;
  UI::UpdateWindowTitle(project);
  EXPECT_EQ(GetLastWindowTitle(), "Desktop-D30: test_project.d30");
}

TEST_F(UITest, UpdateWindowTitle_Dirty) {
  project.projectFilePath = "/home/user/test_project.d30";
  project.isDirty = true;
  UI::UpdateWindowTitle(project);
  EXPECT_EQ(GetLastWindowTitle(), "Desktop-D30: *test_project.d30");
}

TEST_F(UITest, UpdateWindowTitle_WindowsPath) {
    project.projectFilePath = "C:\\Users\\User\\test.d30";
    project.isDirty = false;
    UI::UpdateWindowTitle(project);
    EXPECT_EQ(GetLastWindowTitle(), "Desktop-D30: test.d30");
}