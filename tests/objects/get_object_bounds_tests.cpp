#include "objects.h"
#include "types.h"
#include <gtest/gtest.h>

TEST(GetObjectBoundsTest, TextObject) {
    LabelObject obj = OBJECTS::CreateTextObject(10, 20, "Hello", 24);
    // For text, width is calculated based on font, so we can't easily test it here.
    // We will test the position and that the size is not zero.
    Rectangle bounds = OBJECTS::GetObjectBounds(obj);
    EXPECT_FLOAT_EQ(bounds.x, 10);
    EXPECT_FLOAT_EQ(bounds.y, 20);
    EXPECT_GT(bounds.width, 0);
    EXPECT_GT(bounds.height, 0);
}

TEST(GetObjectBoundsTest, QRCodeObject) {
    LabelObject obj = OBJECTS::CreateQRCodeObject(30, 40, 100, "test");
    Rectangle bounds = OBJECTS::GetObjectBounds(obj);
    EXPECT_FLOAT_EQ(bounds.x, 30);
    EXPECT_FLOAT_EQ(bounds.y, 40);
    EXPECT_FLOAT_EQ(bounds.width, 100);
    EXPECT_FLOAT_EQ(bounds.height, 100);
}

TEST(GetObjectBoundsTest, ImageObject) {
    LabelObject obj = OBJECTS::CreateImageObject(50, 60, 150, 120, "path/to/image.png");
    Rectangle bounds = OBJECTS::GetObjectBounds(obj);
    EXPECT_FLOAT_EQ(bounds.x, 50);
    EXPECT_FLOAT_EQ(bounds.y, 60);
    EXPECT_FLOAT_EQ(bounds.width, 150);
    EXPECT_FLOAT_EQ(bounds.height, 120);
}

TEST(GetObjectBoundsTest, LineObject) {
    LabelObject obj = OBJECTS::CreateLineObject(70, 80, 100, 50, 4);
    Rectangle bounds = OBJECTS::GetObjectBounds(obj);
    EXPECT_FLOAT_EQ(bounds.x, 70);
    EXPECT_FLOAT_EQ(bounds.y, 80);
    EXPECT_FLOAT_EQ(bounds.width, 100);
    EXPECT_FLOAT_EQ(bounds.height, 50);
}

TEST(GetObjectBoundsTest, RectangleObject) {
    LabelObject obj = OBJECTS::CreateRectangleObject(90, 100, 200, 150, 10);
    Rectangle bounds = OBJECTS::GetObjectBounds(obj);
    EXPECT_FLOAT_EQ(bounds.x, 90);
    EXPECT_FLOAT_EQ(bounds.y, 100);
    EXPECT_FLOAT_EQ(bounds.width, 200);
    EXPECT_FLOAT_EQ(bounds.height, 150);
}

TEST(GetObjectBoundsTest, CircleObject) {
    LabelObject obj = OBJECTS::CreateCircleObject(110, 120, 50);
    Rectangle bounds = OBJECTS::GetObjectBounds(obj);
    EXPECT_FLOAT_EQ(bounds.x, 110);
    EXPECT_FLOAT_EQ(bounds.y, 120);
    EXPECT_FLOAT_EQ(bounds.width, 100);
    EXPECT_FLOAT_EQ(bounds.height, 100);
}

TEST(GetObjectBoundsTest, BarcodeObject) {
    LabelObject obj = OBJECTS::CreateBarcodeObject(130, 140, 250, 80, "12345");
    Rectangle bounds = OBJECTS::GetObjectBounds(obj);
    EXPECT_FLOAT_EQ(bounds.x, 130);
    EXPECT_FLOAT_EQ(bounds.y, 140);
    EXPECT_FLOAT_EQ(bounds.width, 250);
    EXPECT_FLOAT_EQ(bounds.height, 80);
}
