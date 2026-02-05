#include "utils.h"
#include "protocol.h"
#include "types.h"
#include <gtest/gtest.h>

namespace {
int printLabelCallCount = 0;
Project lastPrintedProject;

void MockPrintLabel(const Project &project) {
    printLabelCallCount++;
    lastPrintedProject = project;
}
}

class BatchPrintTest : public ::testing::Test {
protected:
    void SetUp() override {
        printLabelCallCount = 0;
        Protocol::SetPrintLabelFunc(MockPrintLabel);
    }
};

TEST_F(BatchPrintTest, PrintsCorrectNumberOfLabels) {
    Project project;
    project.csvHeaders = {"Name", "ID"};
    project.csvRows = {
        {"First", "1"},
        {"Second", "2"},
        {"Third", "3"},
    };

    LabelObject nameObj;
    nameObj.linkedColumn = "Name";
    project.objects.push_back(nameObj);

    Utils::BatchPrint(project, 1, 3);
    EXPECT_EQ(printLabelCallCount, 3);
}

TEST_F(BatchPrintTest, PrintsCorrectData) {
    Project project;
    project.csvHeaders = {"Name", "ID"};
    project.csvRows = {
        {"First", "1"},
        {"Second", "2"},
    };

    LabelObject nameObj;
    nameObj.linkedColumn = "Name";
    project.objects.push_back(nameObj);
    
    LabelObject idObj;
    idObj.linkedColumn = "ID";
    project.objects.push_back(idObj);

    // Test first row
    Utils::BatchPrint(project, 1, 1);
    EXPECT_EQ(printLabelCallCount, 1);
    EXPECT_EQ(lastPrintedProject.objects[0].data, "First");
    EXPECT_EQ(lastPrintedProject.objects[1].data, "1");

    // Reset and test second row
    printLabelCallCount = 0;
    Utils::BatchPrint(project, 2, 2);
    EXPECT_EQ(printLabelCallCount, 1);
    EXPECT_EQ(lastPrintedProject.objects[0].data, "Second");
    EXPECT_EQ(lastPrintedProject.objects[1].data, "2");
}
