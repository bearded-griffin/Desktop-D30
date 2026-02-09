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
 * @file     include/protocol.h
 * @brief    Defines the communication to the printer.
 * @details  Contains the small details for how to
 * actually talk to the D30 printer.
 * @note
 * @date     2026.01.20
 * @author   bearded.griffin
 ****************************************************/

#pragma once
#include "raylib.h"
#include "types.h"
#include <vector>

namespace Protocol {

// Function pointer for mocking
using PrintLabelFunc = void (*)(const Project &);
void SetPrintLabelFunc(PrintLabelFunc func);

// Takes the project, renders it to an image, converts it to
// printer-ready bytes, and sends it via the Printer class.
void PrintLabel(const Project &project);

// Exposed for testing
void ApplyDithering(Image &image);

} // namespace Protocol