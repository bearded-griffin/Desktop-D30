#pragma once
#include "types.h" // We need this so 'Project' is defined

namespace UI {
    // OLD: void DrawSidebar(std::vector<LabelObject>& objects, int& selectedIndex);
    // NEW: Accepts the whole Project struct
    void DrawSidebar(Project& project, int& selectedIndex);
    
    // OLD: void DrawMainMenu();
    // NEW: Accepts Project so we can call Save/Load
    void DrawMainMenu(Project& project);
}