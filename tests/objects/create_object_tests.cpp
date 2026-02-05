#include "objects.h"
#include "types.h"
#include <gtest/gtest.h>

TEST(CreateObjectTest, CreateTextObject) {
    LabelObject obj = OBJECTS::CreateTextObject(10, 20, "Hello", 24);
    EXPECT_EQ(obj.type, ObjectType::Text);
    EXPECT_EQ(obj.x, 10);
    EXPECT_EQ(obj.y, 20);
    EXPECT_EQ(obj.data, "Hello");
    EXPECT_EQ(obj.fontSize, 24);
    EXPECT_EQ(obj.colorHex, 0x000000FF);
}

TEST(CreateObjectTest, CreateQRCodeObject) {
    LabelObject obj = OBJECTS::CreateQRCodeObject(30, 40, 100, "test");
    EXPECT_EQ(obj.type, ObjectType::QRCode);
    EXPECT_EQ(obj.x, 30);
    EXPECT_EQ(obj.y, 40);
    EXPECT_EQ(obj.width, 100);
    EXPECT_EQ(obj.height, 100);
    EXPECT_EQ(obj.data, "test");
    EXPECT_EQ(obj.colorHex, 0x000000FF);
}

TEST(CreateObjectTest, CreateImageObject) {
    LabelObject obj = OBJECTS::CreateImageObject(50, 60, 150, 120, "path/to/image.png");
    EXPECT_EQ(obj.type, ObjectType::Image);
    EXPECT_EQ(obj.x, 50);
    EXPECT_EQ(obj.y, 60);
    EXPECT_EQ(obj.width, 150);
    EXPECT_EQ(obj.height, 120);
    EXPECT_EQ(obj.data, "path/to/image.png");
    EXPECT_EQ(obj.colorHex, 0x000000FF);
}

TEST(CreateObjectTest, CreateLineObject) {
    LabelObject obj = OBJECTS::CreateLineObject(70, 80, 100, 50, 4);
    EXPECT_EQ(obj.type, ObjectType::Line);
    EXPECT_EQ(obj.x, 70);
    EXPECT_EQ(obj.y, 80);
    EXPECT_EQ(obj.width, 100);
    EXPECT_EQ(obj.height, 50);
    EXPECT_EQ(obj.fontSize, 4);
    EXPECT_EQ(obj.colorHex, 0x000000FF);
}

TEST(CreateObjectTest, CreateRectangleObject) {
    LabelObject obj = OBJECTS::CreateRectangleObject(90, 100, 200, 150, 10);
    EXPECT_EQ(obj.type, ObjectType::ShapeRect);
    EXPECT_EQ(obj.x, 90);
    EXPECT_EQ(obj.y, 100);
    EXPECT_EQ(obj.width, 200);
    EXPECT_EQ(obj.height, 150);
    EXPECT_EQ(obj.cornerRadius, 10);
    EXPECT_EQ(obj.colorHex, 0x000000FF);
}

TEST(CreateObjectTest, CreateCircleObject) {
    LabelObject obj = OBJECTS::CreateCircleObject(110, 120, 50);
    EXPECT_EQ(obj.type, ObjectType::ShapeCircle);
    EXPECT_EQ(obj.x, 110);
    EXPECT_EQ(obj.y, 120);
    EXPECT_EQ(obj.width, 100);
    EXPECT_EQ(obj.height, 100);
    EXPECT_EQ(obj.colorHex, 0x000000FF);
}

TEST(CreateObjectTest, CreateBarcodeObject) {
    LabelObject obj = OBJECTS::CreateBarcodeObject(130, 140, 250, 80, "12345");
    EXPECT_EQ(obj.type, ObjectType::Barcode);
    EXPECT_EQ(obj.x, 130);
    EXPECT_EQ(obj.y, 140);
    EXPECT_EQ(obj.width, 250);
    EXPECT_EQ(obj.height, 80);
    EXPECT_EQ(obj.data, "12345");
    EXPECT_EQ(obj.colorHex, 0x000000FF);
}
