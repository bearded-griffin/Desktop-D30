#pragma once
#include <string>
#include <utility>
#include <vector>

struct FontInfo {
  std::string family;
  std::string style;
  std::string file;
  // Optional: void* imgui_font = nullptr;  // We'll add ImFont* later in your
  // app code
};

std::vector<FontInfo> get_system_fonts(bool filter_latin_only = true);
