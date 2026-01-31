/*!***************************************************
 * @file     protocol.cpp
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

// Takes the project, renders it to an image, converts it to
// printer-ready bytes, and sends it via the Printer class.
void PrintLabel(const Project &project);

} // namespace Protocol