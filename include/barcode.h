//  This file is part of Desktop-D30
//  Copyright (C) 2026 Chris Griffin (bearded-griffin)
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation version 3 of the License.
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <https://www.gnu.org/licenses/>.

/*!***************************************************
 * @file     include/barcode.h
 * @brief    Barcode 128-B Support
 * @details  Generates a 128-B barcode to put on the
 * label.
 * @note
 * @date     2026.01.24
 * @author   bearded.griffin
 ****************************************************/

#pragma once
#include <iostream>
#include <string>
#include <vector>

// Simple Code 128-B Generator (Supports A-Z, a-z, 0-9, and symbols)
namespace Barcode {

// Returns a pattern string where '1' is a black bar and '0' is white space.
inline std::string Encode128(const std::string &input) {
  // Table for Code 128-B (Partial for brevity, covers standard ASCII 32-126)
  // Each character is 11 modules wide.
  static const std::string patterns[] = {
      "11011001100", "11001101100", "11001100110",
      "10010011000", "10010001100", // " !\"#$"
      "10001001100", "10011001000", "10011000100",
      "10001100100", "11001001000", // "%&'()"
      "11001000100", "11000100100", "10110011100",
      "10011011100", "10011001110", // "*+,-."
      "10111001100", "10011101100", "10011100110",
      "11001110010", "11001011100", // "/0123"
      "11001001110", "11011100100", "11001110100",
      "11101101110", "11101001100", // "45678"
      "11100101100", "11100100110", "11101100100",
      "11100110100", "11100110010", // "9:;<="
      "11011011000", "11011000110", "11000110110",
      "10100011000", "10001011000", // ">?@AB"
      "10001000110", "10110001000", "10001101000",
      "10001100010", "11010001000", // "CDEFG"
      "11000101000", "11000100010", "10110111000",
      "10110001110", "10001101110", // "HIJKL"
      "10111011000", "10111000110", "10001110110",
      "11101110110", "11010001110", // "MNOPQ"
      "11000101110", "11011101000", "11011100010",
      "11011101110", "11101011000", // "RSTUV"
      "11101000110", "11100010110", "11101101000",
      "11101100010", "11100011010", // "WXYZ["
      "11101111010", "11001000010", "11110001010",
      "10100110000", "10100001100", // "\\]^_"
      "10010110000", "10010000110", "10000101100",
      "10000100110", "10110010000", // "`abcd"
      "10110000100", "10011010000", "10011000010",
      "10000110100", "10000110010", // "efghi"
      "11000010010", "11001010000", "11110111010",
      "11000010100", "10001111010", // "jklmn"
      "10100111100", "10010111100", "10010011110",
      "10111100100", "10011110100", // "opqrs"
      "10011110010", "11110100100", "11110010100",
      "11110010010", "11011011110", // "tuvwx"
      "11011110110", "11110110110", "10101111000",
      "10100011110", "10001011110" // "yz{|}"
  };

  // Start Code B (104)
  std::string res = "11010010000";
  int sum = 104;
  int pos = 1;

  for (char c : input) {
    int val = c - 32; // Map ASCII to table index
    if (val < 0 || val > 94)
      val = 0; // Fallback to space
    res += patterns[val];
    sum += (val * pos);
    pos++;
  }

  // Checksum
  int check = sum % 103;
  res += patterns[check];

  // Stop Character
  res += "1100011101011";

  return res;
}
} // namespace Barcode