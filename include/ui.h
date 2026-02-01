/*!***************************************************
 * @file     ui.h
 * @brief    Handels all UI operations
 * @details  draws the menus, and canvas.
 * @note
 * @date     2026.01.19
 * @author   bearded.griffin
 ****************************************************/

#pragma once
#include "types.h"

namespace UI {
void DrawSidebar(Project &project, int &selectedIndex);
void DrawMainMenu(Project &project);
void RequestExit();
bool ShouldClose();
void ClearExitRequest();
void DrawExitConfirmation(Project &project);
void DrawLoadConfirmation(Project &project);
} // namespace UI