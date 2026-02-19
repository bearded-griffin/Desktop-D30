#include "rendering.h"
#include "objects.h"
#include "types.h"
#include <gtest/gtest.h>

// --- Helper Functions from mocks.cpp ---
void ResetMockState();
int GetDrawTextExCount();
std::string GetLastDrawTextExString();
Vector2 GetLastDrawTextExPos();

int GetDrawTextProCount();
std::string GetLastDrawTextProString();
Vector2 GetLastDrawTextProPos();

int GetDrawLineExCount();
Vector2 GetLastDrawLineExStart();
Vector2 GetLastDrawLineExEnd();

int GetDrawRectangleRoundedCount();
Rectangle GetLastDrawRectangleRoundedRec();

int GetDrawRingCount();
Vector2 GetLastDrawRingCenter();
float GetLastDrawRingRadiusOuter();

int GetDrawRectangleCount();
int GetDrawRectangleLinesCount();
int GetDrawCircleCount();
int GetDrawTextureProCount();


class RenderingTest : public ::testing::Test {
protected:
    void SetUp() override {
        ResetMockState();
        camera = {0};
        camera.zoom = 1.0f;
    }

    Camera2D camera;
};

TEST_F(RenderingTest, RenderTextObject_DrawsText) {
    LabelObject obj = OBJECTS::CreateTextObject(10, 20, "TestRender", 12);
    // Ensure fontName is empty so it gets default font which is fine for mocks
    
    RENDERING::RenderObject(obj, false, camera);
    
    EXPECT_EQ(GetDrawTextProCount(), 1);
    EXPECT_EQ(GetLastDrawTextProString(), "TestRender");
    // DrawTextPro pos might be offset by wrapping logic, but usually starts at x,y
    EXPECT_FLOAT_EQ(GetLastDrawTextProPos().x, 10);
    EXPECT_FLOAT_EQ(GetLastDrawTextProPos().y, 20);
}

TEST_F(RenderingTest, RenderLineObject_DrawsLine) {
    LabelObject obj = OBJECTS::CreateLineObject(10, 10, 100, 50, 5);
    
    RENDERING::RenderObject(obj, false, camera);
    
    EXPECT_EQ(GetDrawLineExCount(), 1);
    EXPECT_FLOAT_EQ(GetLastDrawLineExStart().x, 10);
    EXPECT_FLOAT_EQ(GetLastDrawLineExStart().y, 10);
    EXPECT_FLOAT_EQ(GetLastDrawLineExEnd().x, 110); // 10 + 100
    EXPECT_FLOAT_EQ(GetLastDrawLineExEnd().y, 60);  // 10 + 50
}

TEST_F(RenderingTest, RenderShapeRect_DrawsRoundedRect) {
    // cornerRadius=5, width=50, height=40. CreateRectangleObject sets fontSize=4 (thickness)
    LabelObject obj = OBJECTS::CreateRectangleObject(20, 20, 50, 40, 5);
    
    RENDERING::RenderObject(obj, false, camera);
    
    // It draws the outer rect then the inner rect (to make it hollow)
    EXPECT_EQ(GetDrawRectangleRoundedCount(), 2);
    Rectangle rec = GetLastDrawRectangleRoundedRec(); // This will be the inner rect
    // Outer: {20, 20, 50, 40}
    // Inner: {20+4, 20+4, 50-8, 40-8} -> {24, 24, 42, 32}
    EXPECT_FLOAT_EQ(rec.x, 24);
    EXPECT_FLOAT_EQ(rec.y, 24);
    EXPECT_FLOAT_EQ(rec.width, 42);
    EXPECT_FLOAT_EQ(rec.height, 32);
}

TEST_F(RenderingTest, RenderShapeCircle_DrawsRing) {
    // Circle creation: CreateCircleObject(x, y, radius) -> width=radius*2, height=radius*2
    LabelObject obj = OBJECTS::CreateCircleObject(30, 30, 25);
    
    RENDERING::RenderObject(obj, false, camera);
    
    EXPECT_EQ(GetDrawRingCount(), 1);
    Vector2 center = GetLastDrawRingCenter();
    // Center is x + radius, y + radius -> 30 + 25 = 55
    EXPECT_FLOAT_EQ(center.x, 55);
    EXPECT_FLOAT_EQ(center.y, 55);
    EXPECT_FLOAT_EQ(GetLastDrawRingRadiusOuter(), 25);
}

TEST_F(RenderingTest, RenderBarcode_DrawsBars) {
    LabelObject obj = OBJECTS::CreateBarcodeObject(10, 10, 100, 50, "123");
    
    RENDERING::RenderObject(obj, false, camera);
    
    // Barcode drawing now uses a cached texture
    // We'll update mocks to track DrawTexturePro
    EXPECT_EQ(GetDrawTextureProCount(), 1);
    // And an outline
    EXPECT_EQ(GetDrawRectangleLinesCount(), 1);
}

TEST_F(RenderingTest, DrawTextBox_Wrapping) {
    // Setup a width that forces wrapping.
    // Mock MeasureTextEx returns length * (fontSize/2).
    // Word "Hello" (length 5) -> 5 * (12/2) = 30 width.
    // Word "World" (length 5) -> 30 width.
    // Space " " -> 1 * 6 = 6 width.
    // Total "Hello World" -> 30 + 6 + 30 = 66.
    
    // If maxWidth is 40, "Hello" fits (30 < 40).
    // "World" (starts at 36) -> 36 + 30 = 66 > 40. Should wrap.
    
    Font font = {0};
    RENDERING::DrawTextBox(nullptr, font, "Hello World", 0, 0, 12, 1, WHITE, 40);
    
    EXPECT_EQ(GetDrawTextProCount(), 2);
    // Logic: First word drawn at 0,0. Second word drawn at 0, 12 (newline).
}

TEST_F(RenderingTest, RenderSelectionHandles) {
    LabelObject obj = OBJECTS::CreateRectangleObject(10, 10, 50, 50);
    
    // Select it
    RENDERING::RenderObject(obj, true, camera);
    
    EXPECT_EQ(GetDrawCircleCount(), 8); // 4 filled + 4 lines
    EXPECT_EQ(GetDrawLineExCount(), 9);   // 4 for box + 2 for X + 3 for arrows
    EXPECT_EQ(GetDrawRectangleLinesCount(), 0); // Outline is now drawn via LineEx for rotation
}

TEST_F(RenderingTest, RenderProjectToImage_CreatesValidImage) {
    Project p;
    p.selectedLabelIndex = 0; // 240x96
    p.objects.push_back(OBJECTS::CreateTextObject(10, 10, "Test", 20));
    
    Image img = RENDERING::RenderProjectToImage(p);
    
    EXPECT_EQ(img.width, 240);
    EXPECT_EQ(img.height, 96);
    
    // Cleanup mock allocation
    UnloadImage(img);
}
