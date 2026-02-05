#include "utils.h"
#include "types.h"
#include <gtest/gtest.h>
#include <fstream>
#include <cstdio>
#include <cstring>  // For strcpy (though not strictly needed for direct char* to string assignment here)
#include <unistd.h> // For mkstemp, close

class LoadCSVTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a temporary file for testing using mkstemp
        // Using a template for mkstemp to create a secure temporary file
        char filename_template[] = "/tmp/load_csv_test_XXXXXX";
        int fd = mkstemp(filename_template);
        if (fd != -1) {
            close(fd); // Close the file descriptor, we only need the name
            tmp_filename = filename_template;
        } else {
            // If mkstemp fails, the test should fail
            tmp_filename = ""; // Ensure it's empty to prevent std::ofstream from trying to open an invalid path
            FAIL() << "Failed to create temporary file with mkstemp: " << strerror(errno);
        }
    }

    void TearDown() override {
        // Clean up the temporary file
        if (!tmp_filename.empty()) {
            std::remove(tmp_filename.c_str());
        }
    }

    std::string tmp_filename;
};

TEST_F(LoadCSVTest, LoadsValidCSV) {
    std::ofstream tmp_file(tmp_filename);
    tmp_file << "Header1,Header2,Header3\n";
    tmp_file << "r1c1,r1c2,r1c3\n";
    tmp_file << "r2c1,r2c2,r2c3\n";
    tmp_file.close();

    Project project;
    bool result = Utils::LoadCSV(tmp_filename, project);

    ASSERT_TRUE(result);
    ASSERT_EQ(project.csvHeaders.size(), 3);
    EXPECT_EQ(project.csvHeaders[0], "Header1");
    EXPECT_EQ(project.csvHeaders[1], "Header2");
    EXPECT_EQ(project.csvHeaders[2], "Header3");

    ASSERT_EQ(project.csvRows.size(), 2);
    ASSERT_EQ(project.csvRows[0].size(), 3);
    EXPECT_EQ(project.csvRows[0][0], "r1c1");
    EXPECT_EQ(project.csvRows[0][1], "r1c2");
    EXPECT_EQ(project.csvRows[0][2], "r1c3");
    ASSERT_EQ(project.csvRows[1].size(), 3);
    EXPECT_EQ(project.csvRows[1][0], "r2c1");
    EXPECT_EQ(project.csvRows[1][1], "r2c2");
    EXPECT_EQ(project.csvRows[1][2], "r2c3");
}

TEST_F(LoadCSVTest, HandlesEmptyFile) {
    std::ofstream tmp_file(tmp_filename);
    tmp_file.close();

    Project project;
    bool result = Utils::LoadCSV(tmp_filename, project);

    ASSERT_TRUE(result);
    EXPECT_TRUE(project.csvHeaders.empty());
    EXPECT_TRUE(project.csvRows.empty());
}

TEST_F(LoadCSVTest, HandlesFileWithOnlyHeader) {
    std::ofstream tmp_file(tmp_filename);
    tmp_file << "Header1,Header2\n";
    tmp_file.close();

    Project project;
    bool result = Utils::LoadCSV(tmp_filename, project);

    ASSERT_TRUE(result);
    ASSERT_EQ(project.csvHeaders.size(), 2);
    EXPECT_EQ(project.csvHeaders[0], "Header1");
    EXPECT_EQ(project.csvHeaders[1], "Header2");
    EXPECT_TRUE(project.csvRows.empty());
}

TEST_F(LoadCSVTest, HandlesMismatchedColumns) {
    std::ofstream tmp_file(tmp_filename);
    tmp_file << "Header1,Header2\n";
    tmp_file << "r1c1\n"; // Mismatched row
    tmp_file << "r2c1,r2c2\n";
    tmp_file.close();

    Project project;
    bool result = Utils::LoadCSV(tmp_filename, project);

    ASSERT_TRUE(result);
    ASSERT_EQ(project.csvRows.size(), 1); // Should skip the mismatched row
    ASSERT_EQ(project.csvRows[0].size(), 2);
    EXPECT_EQ(project.csvRows[0][0], "r2c1");
    EXPECT_EQ(project.csvRows[0][1], "r2c2");
}

TEST_F(LoadCSVTest, HandlesNonexistentFile) {
    Project project;
    bool result = Utils::LoadCSV("nonexistent_file.csv", project);
    ASSERT_FALSE(result);
}
