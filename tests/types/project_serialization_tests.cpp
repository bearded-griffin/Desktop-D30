#include "types.h"
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

TEST(ProjectSerializationTest, FullProjectRoundTrip) {
    Project p;
    p.version = 2;
    p.selectedLabelIndex = 3;
    p.projectFilePath = "my_project.d30";
    
    LabelObject obj;
    obj.type = ObjectType::QRCode;
    obj.x = 10.5f;
    obj.y = 20.5f;
    obj.data = "https://example.com";
    obj.fontSize = 50.0f;
    obj.colorHex = 0xFF0000FF;
    p.objects.push_back(obj);
    
    p.csvHeaders = {"Name", "ID"};
    p.csvRows = {{"Alice", "001"}, {"Bob", "002"}};
    p.csvFilePath = "data.csv";
    p.currentCSVRow = 1;

    // Serialize
    nlohmann::json j = p;
    
    // Deserialize
    Project p2 = j.get<Project>();
    
    EXPECT_EQ(p2.version, p.version);
    EXPECT_EQ(p2.selectedLabelIndex, p.selectedLabelIndex);
    EXPECT_EQ(p2.projectFilePath, p.projectFilePath);
    
    ASSERT_EQ(p2.objects.size(), 1);
    EXPECT_EQ(p2.objects[0].type, p.objects[0].type);
    EXPECT_FLOAT_EQ(p2.objects[0].x, p.objects[0].x);
    EXPECT_FLOAT_EQ(p2.objects[0].y, p.objects[0].y);
    EXPECT_EQ(p2.objects[0].data, p.objects[0].data);
    EXPECT_EQ(p2.objects[0].colorHex, p.objects[0].colorHex);
    
    ASSERT_EQ(p2.csvHeaders.size(), 2);
    EXPECT_EQ(p2.csvHeaders[0], "Name");
    
    ASSERT_EQ(p2.csvRows.size(), 2);
    EXPECT_EQ(p2.csvRows[0][0], "Alice");
    EXPECT_EQ(p2.csvFilePath, p.csvFilePath);
    EXPECT_EQ(p2.currentCSVRow, p.currentCSVRow);
}
