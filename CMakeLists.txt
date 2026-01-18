cmake_minimum_required(VERSION 3.14)
project(LabelForge VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# --- 1. Fetch Raylib ---
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG master
)
FetchContent_MakeAvailable(raylib)

# --- 2. Fetch Dear ImGui ---
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG docking
)
FetchContent_MakeAvailable(imgui)

# --- 3. Fetch rlImGui (The Bridge) ---
# We fetch it directly so we don't need manual files in a vendor folder
FetchContent_Declare(
    rlImGui
    GIT_REPOSITORY https://github.com/raylib-extras/rlImGui.git
    GIT_TAG main
)
FetchContent_MakeAvailable(rlImGui)

# --- 4. Fetch nlohmann/json ---
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.2
)
FetchContent_MakeAvailable(json)

# --- 5. Setup Executable ---
add_executable(LabelForge
    src/main.cpp
    src/utils.cpp
    src/ui.cpp
)

# --- 6. Include Paths ---
# This tells C++ where to look for .h files
target_include_directories(LabelForge PRIVATE 
    include/ 
    ${rlImGui_SOURCE_DIR} # Access to rlImGui.h
)

# --- 7. Link Libraries ---
target_link_libraries(LabelForge PRIVATE 
    raylib 
    imgui 
    rlImGui 
    nlohmann_json::nlohmann_json
)

# System specific linking
if (WIN32)
    target_link_libraries(LabelForge PRIVATE winmm)
elseif (UNIX AND NOT APPLE)
    target_link_libraries(LabelForge PRIVATE m pthread dl)
endif()