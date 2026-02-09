#include "objects.h"
#include "types.h"
#include "raylib.h"
#include <gtest/gtest.h>

// External helpers from mocks.cpp
void SetMockMouseDelta(Vector2 delta);
void SetMockShiftDown(bool down);

class ObjectInteractionTest : public ::testing::Test {
protected:
    void SetUp() override {
        project = Project();
        project.selectedLabelIndex = 0; // "12mm x 30mm" (240x96)
        
        selectedIndex = -1;
        isDraggingObject = false;
        dragOffset = {0, 0};
        camera = {0};
        camera.zoom = 1.0f;
    }

    Project project;
    int selectedIndex;
    bool isDraggingObject;
    Vector2 dragOffset;
    Camera2D camera;
};

TEST_F(ObjectInteractionTest, HandleObjectResize_LineObject) {
    project.objects.push_back(OBJECTS::CreateLineObject(10, 10, 100, 0, 4));
    selectedIndex = 0;
    Vector2 mouseWorld = {150, 60};
    
    OBJECTS::HandleObjectResize(project, selectedIndex, HANDLE_BOTTOM_RIGHT, mouseWorld, camera);
    
    EXPECT_FLOAT_EQ(project.objects[0].width, 140); // 150 - 10
    EXPECT_FLOAT_EQ(project.objects[0].height, 50); // 60 - 10
}

TEST_F(ObjectInteractionTest, HandleObjectResize_LineObjectShift) {
    project.objects.push_back(OBJECTS::CreateLineObject(10, 10, 100, 0, 4));
    selectedIndex = 0;
    SetMockShiftDown(true);
    Vector2 mouseWorld = {150, 20}; // Mostly horizontal
    
    OBJECTS::HandleObjectResize(project, selectedIndex, HANDLE_BOTTOM_RIGHT, mouseWorld, camera);
    
    EXPECT_FLOAT_EQ(project.objects[0].width, 140);
    EXPECT_FLOAT_EQ(project.objects[0].height, 0); // Snapped to horizontal
    SetMockShiftDown(false);
}

TEST_F(ObjectInteractionTest, HandleObjectSelection_SelectsObject) {
    project.objects.push_back(OBJECTS::CreateRectangleObject(10, 10, 50, 50));
    
    Vector2 mouseWorld = {35, 35}; // Middle of the object
    OBJECTS::HandleObjectSelection(project, selectedIndex, isDraggingObject, dragOffset, mouseWorld, camera);
    
    EXPECT_EQ(selectedIndex, 0);
    EXPECT_TRUE(isDraggingObject);
    EXPECT_FLOAT_EQ(dragOffset.x, 25); // 35 - 10
    EXPECT_FLOAT_EQ(dragOffset.y, 25); // 35 - 10
}

TEST_F(ObjectInteractionTest, HandleObjectSelection_SelectsTopMostObject) {
    project.objects.push_back(OBJECTS::CreateRectangleObject(10, 10, 50, 50)); // Index 0
    project.objects.push_back(OBJECTS::CreateRectangleObject(20, 20, 50, 50)); // Index 1 (Top)
    
    Vector2 mouseWorld = {35, 35}; // Overlaps both, but index 1 is on top
    OBJECTS::HandleObjectSelection(project, selectedIndex, isDraggingObject, dragOffset, mouseWorld, camera);
    
    EXPECT_EQ(selectedIndex, 1);
}

TEST_F(ObjectInteractionTest, HandleObjectSelection_MissesObject) {
    project.objects.push_back(OBJECTS::CreateRectangleObject(10, 10, 50, 50));
    
    Vector2 mouseWorld = {100, 100};
    OBJECTS::HandleObjectSelection(project, selectedIndex, isDraggingObject, dragOffset, mouseWorld, camera);
    
    EXPECT_EQ(selectedIndex, -1);
    EXPECT_FALSE(isDraggingObject);
}

TEST_F(ObjectInteractionTest, HandleObjectDrag_UpdatesPosition) {
    project.objects.push_back(OBJECTS::CreateRectangleObject(10, 10, 50, 50));
    selectedIndex = 0;
    dragOffset = {25, 25};
    Vector2 mouseWorld = {60, 60};
    
    OBJECTS::HandleObjectDrag(project, selectedIndex, mouseWorld, dragOffset, camera);
    
    EXPECT_FLOAT_EQ(project.objects[0].x, 35); // 60 - 25
    EXPECT_FLOAT_EQ(project.objects[0].y, 35); // 60 - 25
    EXPECT_TRUE(project.isDirty);
}

TEST_F(ObjectInteractionTest, HandleObjectDrag_ClampsToCanvas) {
    project.objects.push_back(OBJECTS::CreateRectangleObject(10, 10, 50, 50));
    selectedIndex = 0;
    dragOffset = {25, 25};
    
    // Drag far outside
    Vector2 mouseWorld = {1000, 1000};
    OBJECTS::HandleObjectDrag(project, selectedIndex, mouseWorld, dragOffset, camera);
    
    LabelSize canvasSz = LabelSizes[project.selectedLabelIndex];
    EXPECT_FLOAT_EQ(project.objects[0].x, canvasSz.width - 50);
    EXPECT_FLOAT_EQ(project.objects[0].y, canvasSz.height - 50);
}

TEST_F(ObjectInteractionTest, HandleObjectResize_BottomRight) {
    project.objects.push_back(OBJECTS::CreateRectangleObject(10, 10, 50, 50));
    selectedIndex = 0;
    Vector2 mouseWorld = {100, 100};
    
    OBJECTS::HandleObjectResize(project, selectedIndex, HANDLE_BOTTOM_RIGHT, mouseWorld, camera);
    
    EXPECT_FLOAT_EQ(project.objects[0].width, 90); // 100 - 10
    EXPECT_FLOAT_EQ(project.objects[0].height, 90); // 100 - 10
}

TEST_F(ObjectInteractionTest, HandleObjectResize_TopLeft) {
    project.objects.push_back(OBJECTS::CreateRectangleObject(10, 10, 50, 50));
    selectedIndex = 0;
    Vector2 mouseWorld = {5, 5};
    
    OBJECTS::HandleObjectResize(project, selectedIndex, HANDLE_TOP_LEFT, mouseWorld, camera);
    
    // Bounds before: x=10, y=10, w=50, h=50 -> right=60, bottom=60
    // New x=5, y=5. width = 60 - 5 = 55, height = 60 - 5 = 55
    EXPECT_FLOAT_EQ(project.objects[0].x, 5);
    EXPECT_FLOAT_EQ(project.objects[0].y, 5);
    EXPECT_FLOAT_EQ(project.objects[0].width, 55);
    EXPECT_FLOAT_EQ(project.objects[0].height, 55);
}

TEST_F(ObjectInteractionTest, HandleObjectResize_ShiftMaintainsAspectRatio) {
    project.objects.push_back(OBJECTS::CreateRectangleObject(10, 10, 100, 50)); // 2:1 ratio
    selectedIndex = 0;
    SetMockShiftDown(true);
    Vector2 mouseWorld = {210, 110}; // width becomes 200. height should become 100 to maintain 2:1
    
    OBJECTS::HandleObjectResize(project, selectedIndex, HANDLE_BOTTOM_RIGHT, mouseWorld, camera);
    
    EXPECT_FLOAT_EQ(project.objects[0].width, 200);
    EXPECT_FLOAT_EQ(project.objects[0].height, 100);
    SetMockShiftDown(false);
}

TEST_F(ObjectInteractionTest, HandleObjectSelection_SelectsLine) {
    project.objects.push_back(OBJECTS::CreateLineObject(10, 10, 100, 0, 10)); // Horizontal line
    
    // Near the line
    Vector2 mouseWorld = {50, 12}; 
    OBJECTS::HandleObjectSelection(project, selectedIndex, isDraggingObject, dragOffset, mouseWorld, camera);
    
    EXPECT_EQ(selectedIndex, 0);
}

TEST_F(ObjectInteractionTest, HandleObjectResize_TopRight) {
    project.objects.push_back(OBJECTS::CreateRectangleObject(10, 10, 50, 50));
    selectedIndex = 0;
    Vector2 mouseWorld = {100, 5};
    
    OBJECTS::HandleObjectResize(project, selectedIndex, HANDLE_TOP_RIGHT, mouseWorld, camera);
    
    // x stays 10. width becomes 100 - 10 = 90.
    // y becomes 5. height becomes (10 + 50) - 5 = 55.
    EXPECT_FLOAT_EQ(project.objects[0].x, 10);
    EXPECT_FLOAT_EQ(project.objects[0].y, 5);
    EXPECT_FLOAT_EQ(project.objects[0].width, 90);
    EXPECT_FLOAT_EQ(project.objects[0].height, 55);
}

TEST_F(ObjectInteractionTest, HandleObjectResize_BottomLeft) {
    project.objects.push_back(OBJECTS::CreateRectangleObject(10, 10, 50, 50));
    selectedIndex = 0;
    Vector2 mouseWorld = {5, 100};
    
    OBJECTS::HandleObjectResize(project, selectedIndex, HANDLE_BOTTOM_LEFT, mouseWorld, camera);
    
    // x becomes 5. width becomes (10 + 50) - 5 = 55.
    // y stays 10. height becomes 100 - 10 = 90.
    EXPECT_FLOAT_EQ(project.objects[0].x, 5);
    EXPECT_FLOAT_EQ(project.objects[0].y, 10);
    EXPECT_FLOAT_EQ(project.objects[0].width, 55);
    EXPECT_FLOAT_EQ(project.objects[0].height, 90);
}

TEST_F(ObjectInteractionTest, HandleObjectResize_TextObject) {
    project.objects.push_back(OBJECTS::CreateTextObject(10, 10, "Test", 20));
    selectedIndex = 0;
    SetMockMouseDelta({0, 10}); // Mouse moved down 10 pixels
    
    // Note: TEXT_RESIZE_FACTOR is 0.5f in types.h? Let me check.
    // In src/objects.cpp: obj.fontSize -= (mouseDelta.y * TEXT_RESIZE_FACTOR);
    // If factor is 0.5, then 20 - (10 * 0.5) = 15.
    
    OBJECTS::HandleObjectResize(project, selectedIndex, HANDLE_BOTTOM_RIGHT, {0,0}, camera);
    
    EXPECT_LT(project.objects[0].fontSize, 20);
    SetMockMouseDelta({0, 0});
}
