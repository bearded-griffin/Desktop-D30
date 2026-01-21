/*!***************************************************
 * @file     protocol.cpp
 * @brief    Defines the communication to the printer.
 * @details  Contains the small details for how to 
 * actually talk to the D30 printer.
 * @note     
 * @date     2026.01.20
 * @author   bearded.griffin
 ****************************************************/

#include "protocol.h"
#include "printer.h"
#include "raylib.h"
#include "utils.h"
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

namespace Protocol {

/*!***************************************************
 * @brief    Prints the label...
 * @details  Pulls all the details together to be able
 * to send the label to the printer.
 * @param    project const Project& 
 * @return   void 
 * @note     
 * @date     2026.01.20
 * @author   bearded.griffin
 ****************************************************/
void PrintLabel(const Project &project) {
  if (!Printer::Get().IsConnected()) {
    std::cout << "[Protocol] Cannot print: Printer not connected." << std::endl;
    return;
  }

  std::cout << "[Protocol] Generating Label Data..." << std::endl;
  

  // 1. Get the Source Image (Landscape, e.g., 320x96)
  Image source = Utils::RenderProjectToImage(project);
  ImageFormat(&source, PIXELFORMAT_UNCOMPRESSED_GRAYSCALE);

  // 2. Calculate Dimensions for ROTATION
  // The D30 prints vertically. We must rotate 90 degrees.
  // Source Width becomes Print Height (Lines)
  // Source Height becomes Print Width (Dots)

  int sourceW = source.width;  // e.g. 320
  int sourceH = source.height; // e.g. 96

  // 3. Calculate Dimensions for BITMAP
  int bytesPerRow = (sourceH + 7) / 8;

  
  int paddingLines = 6;
  int safetyBuffer = 20; 
        
  int totalPrintLines = sourceW + paddingLines + safetyBuffer;

  std::vector<uint8_t> pixelData;
  pixelData.reserve(bytesPerRow * totalPrintLines);

  // --- STEP A: PADDING ---
  // Write 6 lines of empty bytes
  for (int p = 0; p < paddingLines; p++) {
    for (int b = 0; b < bytesPerRow; b++) {
      pixelData.push_back(0x00);
    }
  }

  // --- STEP B: BITMAP ROTATION  ---
  // Outer Loop: Iterate Source X from Right -> Left (Rotates the image)
  uint8_t *srcPixels = (uint8_t *)source.data;

  for (int i = 0; i < sourceW; i++) {
    
    int srcX = (sourceW - 1) - i;

    for (int byteIdx = 0; byteIdx < bytesPerRow; byteIdx++) {
      uint8_t currentByte = 0;

      for (int bit = 0; bit < 8; bit++) {
        
        int srcY = (byteIdx * 8) + bit;

        if (srcY < sourceH) {
          // Get Pixel at (srcX, srcY)
          // Raylib Grayscale: < 128 is Dark
          // < 380 (Sum of RGB), which is roughly < 128 grayscale
          if (srcPixels[srcY * sourceW + srcX] < 128) {
            currentByte |= (1 << (7 - bit));
          }
        }
      }
      pixelData.push_back(currentByte);
    }
  }

  // --- STEP C: TRAILING BUFFER ---
  // Fill the rest of the safety buffer with white space
  for (int p = 0; p < safetyBuffer; p++) {
    for (int b = 0; b < bytesPerRow; b++) {
      pixelData.push_back(0x00);
    }
  }

  UnloadImage(source);

  // --- CONSTRUCT PACKET ---
  std::vector<uint8_t> cmd;

  // 1. Phomemo Magic Header
  cmd.push_back(0x1F);
  cmd.push_back(0x11);
  cmd.push_back(0x24);
  cmd.push_back(0x00);

  // 2. ESC/POS Init
  cmd.push_back(0x1B);
  cmd.push_back(0x40);

  // 3. Raster Command (GS v 0)
  cmd.push_back(0x1D);
  cmd.push_back(0x76);
  cmd.push_back(0x30);
  cmd.push_back(0x00);

  // Width (Bytes)
  cmd.push_back(bytesPerRow & 0xFF);
  cmd.push_back((bytesPerRow >> 8) & 0xFF);

  // Height (Total Lines)
  cmd.push_back(totalPrintLines & 0xFF);
  cmd.push_back((totalPrintLines >> 8) & 0xFF);

  // Data
  cmd.insert(cmd.end(), pixelData.begin(), pixelData.end());

  // --- SEND ---
  std::cout << "[Protocol] Sending " << cmd.size() << " bytes..." << std::endl;

  const size_t CHUNK_SIZE = 256;
  size_t offset = 0;

  while (offset < cmd.size()) {
    size_t end = std::min(offset + CHUNK_SIZE, cmd.size());
    std::vector<uint8_t> chunk(cmd.begin() + offset, cmd.begin() + end);

    if (!Printer::Get().Write(chunk)) {
      std::cout << "[Protocol] Failed to send chunk!" << std::endl;
      break;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    offset += CHUNK_SIZE;
  }

  std::cout << "[Protocol] Print job finished." << std::endl;
}
} // namespace Protocol