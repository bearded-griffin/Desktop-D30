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
#include <algorithm>
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

namespace Protocol {

/*!***************************************************
 * @brief    Converts grayscale image to black and white.
 * @details  Uses the Floyd-Steinberg algorithm to
 * convert a grayscale image to strict black & white (1-bit).
 * @param    image Image&
 * @return   void
 * @note
 * @date     2026.01.21
 * @author   bearded.griffin
 ****************************************************/
void ApplyDithering(Image &image) {
  int w = image.width;
  int h = image.height;

  // We need float precision for error accumulation
  // Copy image data to a float buffer
  std::vector<float> pixels(w * h);
  uint8_t *rawData = (uint8_t *)image.data;
  for (int i = 0; i < w * h; i++)
    pixels[i] = (float)rawData[i];

  for (int y = 0; y < h; y++) {
    for (int x = 0; x < w; x++) {
      float oldPixel = pixels[y * w + x];

      // Threshold: If < 128, it's Black (0). Else White (255).
      float newPixel = (oldPixel < 128) ? 0.0f : 255.0f;

      // Calculate Error (How much "Gray" did we lose?)
      float error = oldPixel - newPixel;

      pixels[y * w + x] = newPixel; // Commit the strict B/W value

      // Distribute error to neighbors (Floyd-Steinberg weights)
      // Right (7/16)
      if (x + 1 < w)
        pixels[y * w + (x + 1)] += error * 7.0f / 16.0f;

      // Bottom-Left (3/16)
      if (x - 1 >= 0 && y + 1 < h)
        pixels[(y + 1) * w + (x - 1)] += error * 3.0f / 16.0f;

      // Bottom (5/16)
      if (y + 1 < h)
        pixels[(y + 1) * w + x] += error * 5.0f / 16.0f;

      // Bottom-Right (1/16)
      if (x + 1 < w && y + 1 < h)
        pixels[(y + 1) * w + (x + 1)] += error * 1.0f / 16.0f;
    }
  }

  // Copy back to the Raylib image
  for (int i = 0; i < w * h; i++) {
    // Clamp to ensure safety, though math usually holds up
    rawData[i] = (uint8_t)std::clamp(pixels[i], 0.0f, 255.0f);
  }
}

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

  // 1. Get Source Image
  Image source = Utils::RenderProjectToImage(project);

  // 2. Convert to Grayscale
  ImageFormat(&source, PIXELFORMAT_UNCOMPRESSED_GRAYSCALE);

  // 3. APPLY DITHERING (The Fix)
  // This turns "Orange" (Gray) into "Dots" (Black/White Pattern)
  ApplyDithering(source);

  // 4. Calculate Dimensions for ROTATION (Zig Logic)
  int sourceW = source.width;
  int sourceH = source.height;

  int bytesPerRow = (sourceH + 7) / 8;

  int paddingLines = 6;
  int safetyBuffer = 20;
  int totalPrintLines = sourceW + paddingLines + safetyBuffer;

  std::vector<uint8_t> pixelData;
  pixelData.reserve(bytesPerRow * totalPrintLines);

  // --- STEP A: PADDING ---
  for (int p = 0; p < paddingLines; p++) {
    for (int b = 0; b < bytesPerRow; b++)
      pixelData.push_back(0x00);
  }

  // --- STEP B: BITMAP ROTATION ---
  uint8_t *srcPixels = (uint8_t *)source.data;

  for (int i = 0; i < sourceW; i++) {
    int srcX = (sourceW - 1) - i;

    for (int byteIdx = 0; byteIdx < bytesPerRow; byteIdx++) {
      uint8_t currentByte = 0;

      for (int bit = 0; bit < 8; bit++) {
        int srcY = (byteIdx * 8) + bit;

        if (srcY < sourceH) {
          // Because we ran Dithering, pixels are now strictly 0 or 255.
          // 0 (Black) < 128 -> True.
          if (srcPixels[srcY * sourceW + srcX] < 128) {
            currentByte |= (1 << (7 - bit));
          }
        }
      }
      pixelData.push_back(currentByte);
    }
  }

  UnloadImage(source);

  // --- STEP C: TRAILING BUFFER ---
  for (int p = 0; p < safetyBuffer; p++) {
    for (int b = 0; b < bytesPerRow; b++)
      pixelData.push_back(0x00);
  }

  // --- CONSTRUCT PACKET ---
  std::vector<uint8_t> cmd;

  // Phomemo Magic Header
  cmd.push_back(0x1F);
  cmd.push_back(0x11);
  cmd.push_back(0x24);
  cmd.push_back(0x00);
  // ESC/POS Init
  cmd.push_back(0x1B);
  cmd.push_back(0x40);
  // Raster Command (GS v 0)
  cmd.push_back(0x1D);
  cmd.push_back(0x76);
  cmd.push_back(0x30);
  cmd.push_back(0x00);
  // Width & Height
  cmd.push_back(bytesPerRow & 0xFF);
  cmd.push_back((bytesPerRow >> 8) & 0xFF);
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