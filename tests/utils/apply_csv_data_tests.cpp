#include "utils.h"
#include "types.h"
#include <gtest/gtest.h>

TEST(ApplyCSVDataToObjectsTest, AppliesDataFromCSVRowToObject) {
    Project project;
    project.currentCSVRow = 0;

    // Setup CSV data
    project.csvHeaders = {"Name", "Position", "ID"};
    project.csvRows = {
        {"John Doe", "Developer", "123"},
        {"Jane Smith", "Designer", "456"}
    };

    // Setup objects
    LabelObject nameObject;
    nameObject.linkedColumn = "Name";
    project.objects.push_back(nameObject);

    LabelObject idObject;
    idObject.linkedColumn = "ID";
    project.objects.push_back(idObject);

    // An object with no link
    LabelObject staticObject;
    staticObject.data = "Static";
    project.objects.push_back(staticObject);
    
    // An object with a link that doesn't exist in the CSV
    LabelObject badLinkObject;
    badLinkObject.linkedColumn = "Department";
    badLinkObject.data = "Initial";
    project.objects.push_back(badLinkObject);

    Utils::ApplyCSVDataToObjects(project);

    EXPECT_EQ(project.objects[0].data, "John Doe");
    EXPECT_EQ(project.objects[1].data, "123");
    EXPECT_EQ(project.objects[2].data, "Static");
    EXPECT_EQ(project.objects[3].data, "Initial");
}

TEST(ApplyCSVDataToObjectsTest, HandlesEmptyProject) {
    Project project;
    project.currentCSVRow = 0;
    project.csvHeaders = {"Name"};
    project.csvRows = {{"John Doe"}};

    // Should not crash
    ASSERT_NO_THROW(Utils::ApplyCSVDataToObjects(project));
}

TEST(ApplyCSVDataToObjectsTest, HandlesEmptyCSV) {
    Project project;
    project.currentCSVRow = 0;

    LabelObject nameObject;
    nameObject.linkedColumn = "Name";
    nameObject.data = "Initial";
    project.objects.push_back(nameObject);
    
    // Should not crash and data should not change
    ASSERT_NO_THROW(Utils::ApplyCSVDataToObjects(project));
    EXPECT_EQ(project.objects[0].data, "Initial");
}

TEST(ApplyCSVDataToObjectsTest, HandlesRowIndexOutOfBounds) {
    Project project;
    project.currentCSVRow = 5; // Out of bounds
    project.csvHeaders = {"Name"};
    project.csvRows = {{"John Doe"}};

    LabelObject nameObject;
    nameObject.linkedColumn = "Name";
    nameObject.data = "Initial";
    project.objects.push_back(nameObject);

    // Should not crash and data should not change
    ASSERT_NO_THROW(Utils::ApplyCSVDataToObjects(project));
    EXPECT_EQ(project.objects[0].data, "Initial");
}
