#include "input.h"
#include "types.h"
#include "raylib.h"
#include <gtest/gtest.h>

// Helpers from mocks.cpp
void ResetMockState();
void SetMockMousePosition(Vector2 pos);
void SetMockMouseButtonPressed(int button, bool pressed);
void SetMockMouseButtonReleased(int button, bool released);
void SetMockKeyPressed(int key, bool pressed);
void SetMockMouseWheel(float wheel);

class InputTest : public ::testing::Test {
protected:
    void SetUp() override {
        ResetMockState();
        project = Project();
        project.selectedLabelIndex = 0;
        
        state = InteractionState();
        camera = {0};
        camera.zoom = 1.0f;
    }

    Project project;
    InteractionState state;
    Camera2D camera;
};

TEST_F(InputTest, HandleInput_Selection) {
    project.objects.push_back({ObjectType::ShapeRect, 10, 10, 50, 50, "Box"});
    
    // Click on the box
    SetMockMousePosition({30, 30});
    SetMockMouseButtonPressed(MOUSE_LEFT_BUTTON, true);
    
    INPUT::HandleInput(project, state, camera);
    
    EXPECT_EQ(state.selectedIndex, 0);
    EXPECT_TRUE(state.isDraggingObject);
}

TEST_F(InputTest, HandleInput_Deletion) {
    project.objects.push_back({ObjectType::ShapeRect, 10, 10, 50, 50, "Box"});
    state.selectedIndex = 0;
    
    // Press Delete
    SetMockKeyPressed(KEY_DELETE, true);
    
    INPUT::HandleInput(project, state, camera);
    
    EXPECT_TRUE(project.objects.empty());
    EXPECT_EQ(state.selectedIndex, -1);
    EXPECT_TRUE(project.isDirty);
}

TEST_F(InputTest, HandleInput_Zooming) {
    SetMockMouseWheel(1.0f);
    
    INPUT::HandleInput(project, state, camera);
    
    EXPECT_GT(camera.zoom, 1.0f);
}

TEST_F(InputTest, HandleMouseInteractions_StartsResizing) {
    // A box at 10,10 size 50,50
    project.objects.push_back({ObjectType::ShapeRect, 10, 10, 50, 50, "Box"});
    state.selectedIndex = 0;
    
    // Bottom-right handle position: 60, 60
    SetMockMousePosition({60, 60});
    SetMockMouseButtonPressed(MOUSE_LEFT_BUTTON, true);
    
    INPUT::HandleInput(project, state, camera);
    
    EXPECT_TRUE(state.isResizing);
    EXPECT_EQ(state.activeHandle, HANDLE_BOTTOM_RIGHT);
}

TEST_F(InputTest, HandleMouseInteractions_DeleteViaHandle) {
    project.objects.push_back({ObjectType::ShapeRect, 10, 10, 50, 50, "Box"});
    state.selectedIndex = 0;
    
    // Top-left handle position: 10, 10
    SetMockMousePosition({10, 10});
    SetMockMouseButtonPressed(MOUSE_LEFT_BUTTON, true);
    
    INPUT::HandleInput(project, state, camera);
    
    EXPECT_TRUE(project.objects.empty());
    EXPECT_EQ(state.selectedIndex, -1);
}

TEST_F(InputTest, HandleMouseInteractions_StopDraggingOnRelease) {
    state.isDraggingObject = true;
    state.selectedIndex = 0;
    project.objects.push_back({ObjectType::ShapeRect, 10, 10, 50, 50, "Box"});
    
    SetMockMouseButtonReleased(MOUSE_LEFT_BUTTON, true);
    
    INPUT::HandleInput(project, state, camera);
    
    EXPECT_FALSE(state.isDraggingObject);
}
