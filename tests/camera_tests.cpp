#include "camera.h"
#include "types.h"
#include "utils.h"
#include <gtest/gtest.h>
#include <raylib.h>

// Mock the GetScreenWidth and GetScreenHeight functions
extern "C" {
int mockScreenWidth = 800;
int mockScreenHeight = 600;

int __wrap_GetScreenWidth() { return mockScreenWidth; }
int __wrap_GetScreenHeight() { return mockScreenHeight; }
}

TEST(UpdateCameraTest, SetsOffsetToCenterOfScreen) {
  mockScreenWidth = 1024;
  mockScreenHeight = 768;
  Camera2D camera = {0};
  Project project = {0};

  CAMERA::UpdateCamera(camera, project);

  EXPECT_FLOAT_EQ(camera.offset.x, mockScreenWidth / 2.0f);
  EXPECT_FLOAT_EQ(camera.offset.y, mockScreenHeight / 2.0f);
}

TEST(UpdateCameraTest, UpdatesTargetWhenLabelIndexChanges) {
  Camera2D camera = {0};
  Project project = {0};
  project.selectedLabelIndex = 1; // "12mm x 40mm", 320, 96

  CAMERA::UpdateCamera(camera, project);

  EXPECT_FLOAT_EQ(camera.target.x, 160.0f); // 320/2
  EXPECT_FLOAT_EQ(camera.target.y, 48.0f); // 96/2
}

TEST(UpdateCameraTest, DoesNotUpdateTargetWhenLabelIndexSame) {
  Camera2D camera = {0};
  Project project = {0};
  project.selectedLabelIndex = 1;

  // First update
  CAMERA::UpdateCamera(camera, project);
  float initialTargetX = camera.target.x;
  float initialTargetY = camera.target.y;

  // Second update
  CAMERA::UpdateCamera(camera, project);

  EXPECT_FLOAT_EQ(camera.target.x, initialTargetX);
  EXPECT_FLOAT_EQ(camera.target.y, initialTargetY);
}

TEST(UpdateCameraTest, HandlesLabelIndexChangeFromDifferentValue) {
  Camera2D camera = {0};
  Project project = {0};
  project.selectedLabelIndex = 1;

  // First update with index 1
  CAMERA::UpdateCamera(camera, project);

  // Change to index 2: "14mm x 30mm", 240, 112
  project.selectedLabelIndex = 2;
  CAMERA::UpdateCamera(camera, project);

  EXPECT_FLOAT_EQ(camera.target.x, 120.0f); // 240/2
  EXPECT_FLOAT_EQ(camera.target.y, 56.0f);  // 112/2
}

// This test checks that the camera target is updated correctly for the first label size.
TEST(UpdateCameraTest, HandlesZeroLabelSize) {
  Camera2D camera = {0};
  Project project = {0};
  project.selectedLabelIndex = 0; // "12mm x 30mm", 240, 96

  CAMERA::UpdateCamera(camera, project);

  EXPECT_FLOAT_EQ(camera.target.x, 120.0f); // 240/2
  EXPECT_FLOAT_EQ(camera.target.y, 48.0f); // 96/2
}
TEST(InitCameraTest, InitializesCameraCorrectly) {
  int scrnwidth = 800;
  int scrnheight = 600;
  Camera2D camera = {0};
  Project project = {0};
  project.selectedLabelIndex = 1; // "12mm x 40mm", 320, 96

  CAMERA::InitializeCamera(&scrnwidth, &scrnheight, &camera, &project);

  EXPECT_FLOAT_EQ(camera.zoom, 2.0f);
  EXPECT_FLOAT_EQ(camera.offset.x, scrnwidth / 2.0f);
  EXPECT_FLOAT_EQ(camera.offset.y, scrnheight / 2.0f);
  EXPECT_FLOAT_EQ(camera.target.x, 160.0f); // 320/2
  EXPECT_FLOAT_EQ(camera.target.y, 48.0f); // 96/2
}