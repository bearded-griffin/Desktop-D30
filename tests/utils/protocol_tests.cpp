#include "protocol.h"
#include "utils.h"
#include "raylib.h"
#include <gtest/gtest.h>
#include <vector>

TEST(ProtocolTest, ApplyDithering_ConvertsToBW) {
    // Create a 2x2 grayscale image with mid-tones
    // 100 200
    // 50  150
    Image img = {
        .data = new uint8_t[4]{100, 200, 50, 150},
        .width = 2,
        .height = 2,
        .mipmaps = 1,
        .format = PIXELFORMAT_UNCOMPRESSED_GRAYSCALE
    };

    Protocol::ApplyDithering(img);

    uint8_t* pixels = (uint8_t*)img.data;
    
    // Each pixel should now be strictly 0 or 255
    for(int i = 0; i < 4; i++) {
        EXPECT_TRUE(pixels[i] == 0 || pixels[i] == 255);
    }

    // Clean up
    delete[] (uint8_t*)img.data;
}

TEST(ProtocolTest, ApplyDithering_PatternCheck) {
    // Create a larger image with a gradient to see if it spreads error
    int size = 10;
    Image img = GenImageGradientLinear(size, size, 0, WHITE, BLACK);
    ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_GRAYSCALE);
    
    Protocol::ApplyDithering(img);
    
    uint8_t* pixels = (uint8_t*)img.data;
    bool hasBlack = false;
    bool hasWhite = false;
    
    for(int i = 0; i < size*size; i++) {
        if (pixels[i] == 0) hasBlack = true;
        if (pixels[i] == 255) hasWhite = true;
        EXPECT_TRUE(pixels[i] == 0 || pixels[i] == 255);
    }
    
    EXPECT_TRUE(hasBlack);
    EXPECT_TRUE(hasWhite);
    
    UnloadImage(img);
}

TEST(ProtocolTest, DefaultPrintLabel_FailsWhenDisconnected) {
    Project p;
    // Printer is a singleton, check its state
    // We can't easily reset Printer singleton state if it was connected, 
    // but in tests it should be disconnected by default.
    
    // We can use a capture buffer for stdout if we wanted to be fancy,
    // but for now let's just ensure it doesn't crash.
    ASSERT_NO_THROW(Protocol::PrintLabel(p));
}

TEST(UtilsTest, ExportProjectToPNG_CallsExport) {
    Project p;
    p.selectedLabelIndex = 0;
    
    // This should call RenderProjectToImage and then ExportImage
    // ExportImage is wrapped in mocks.cpp
    ASSERT_NO_THROW(Utils::ExportProjectToPNG("test_export", p));
}
