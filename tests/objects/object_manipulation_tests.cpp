#include "objects.h"
#include "types.h"
#include <gtest/gtest.h>

TEST(ObjectManipulationTest, ClampObjectPosition) {
    LabelObject obj;
    obj.x = -10;
    obj.y = -20;
    obj.width = 50;
    obj.height = 50;

    LabelSize canvasSize = {"Test", 200, 100};

    OBJECTS::ClampObjectPosition(obj, canvasSize);

    EXPECT_FLOAT_EQ(obj.x, 0);
    EXPECT_FLOAT_EQ(obj.y, 0);

    obj.x = 180; // 200 - 50 = 150 is the max x
    obj.y = 80;  // 100 - 50 = 50 is the max y

    OBJECTS::ClampObjectPosition(obj, canvasSize);
    EXPECT_FLOAT_EQ(obj.x, 150);
    EXPECT_FLOAT_EQ(obj.y, 50);
}

TEST(ObjectManipulationTest, ValidateObjectSize_MinSize) {
    LabelObject obj;
    obj.width = 5;
    obj.height = 5;
    obj.fontSize = 5;
    obj.type = ObjectType::Text;

    OBJECTS::ValidateObjectSize(obj);

    EXPECT_FLOAT_EQ(obj.width, MIN_OBJECT_SIZE);
    EXPECT_FLOAT_EQ(obj.height, MIN_OBJECT_SIZE);
    EXPECT_FLOAT_EQ(obj.fontSize, MIN_FONT_SIZE);
}

TEST(ObjectManipulationTest, IsObjectSelected) {
    std::vector<int> selectedIndices = {0, 2};
    
    EXPECT_TRUE(OBJECTS::IsObjectSelected(selectedIndices, 0));
    EXPECT_TRUE(OBJECTS::IsObjectSelected(selectedIndices, 2));
    EXPECT_FALSE(OBJECTS::IsObjectSelected(selectedIndices, 1));
    EXPECT_FALSE(OBJECTS::IsObjectSelected(selectedIndices, -1));
}

TEST(ObjectManipulationTest, ValidateObjectSize_NormalSize) {
    LabelObject obj;
    obj.width = 100;
    obj.height = 100;
    obj.fontSize = 20;
    obj.type = ObjectType::Text;

    OBJECTS::ValidateObjectSize(obj);

    EXPECT_FLOAT_EQ(obj.width, 100);
    EXPECT_FLOAT_EQ(obj.height, 100);
    EXPECT_FLOAT_EQ(obj.fontSize, 20);
}
