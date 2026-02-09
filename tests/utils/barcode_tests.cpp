#include "barcode.h"
#include <gtest/gtest.h>

TEST(BarcodeTest, Encode128_CorrectLength) {
    std::string input = "ABC123";
    std::string encoded = Barcode::Encode128(input);
    
    // Start (11) + 6 chars (6*11) + Checksum (11) + Stop (13) = 11 + 66 + 11 + 13 = 101 modules
    EXPECT_EQ(encoded.length(), 101);
}

TEST(BarcodeTest, Encode128_StartsAndEndsCorrectly) {
    std::string encoded = Barcode::Encode128("TEST");
    
    // Start Code B is 11010010000
    EXPECT_EQ(encoded.substr(0, 11), "11010010000");
    
    // Stop Character + extra bar is 1100011101011
    EXPECT_EQ(encoded.substr(encoded.length() - 13), "1100011101011");
}

TEST(BarcodeTest, Encode128_OnlyZerosAndOnes) {
    std::string encoded = Barcode::Encode128("!@#$%^&*()");
    for(char c : encoded) {
        EXPECT_TRUE(c == '0' || c == '1');
    }
}
