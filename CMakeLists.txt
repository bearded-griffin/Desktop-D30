cmake_minimum_required(VERSION 3.14)
project(LabelForge VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_VERBOSE_BUILD ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
   

include(FetchContent)
set(FETCHCONTENT_QUIET FALSE)

# --- 1. Fetch Raylib ---
message(STATUS "Fetching Raylib...")
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG master
    GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(raylib)

# --- 2. Fetch Dear ImGui ---
message(STATUS "Fetching Dear ImGui...")
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG docking
    GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(imgui)

# --- 3. Fetch rlImGui ---
message(STATUS "Fetching rlImGui...")
FetchContent_Declare(
    rlimgui
    GIT_REPOSITORY https://github.com/raylib-extras/rlImGui.git
    GIT_TAG main
    GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(rlimgui)

# --- 4. Fetch nlohmann/json ---
message(STATUS "Fetching nlohmann/json...")
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.2
    GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(json)

# =========================================================
#  MANUAL LIBRARY GENERATION (The Fix for Linker Errors)
# =========================================================

# 1. Create the 'imgui' Library Target
# We grab the core .cpp files from the fetched directory
file(GLOB IMGUI_SOURCES "${imgui_SOURCE_DIR}/*.cpp")
add_library(imgui STATIC ${IMGUI_SOURCES})
target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR})

# 2. Create the 'rlImGui' Library Target
# We compile the single bridge file and link it to ImGui + Raylib
add_library(rlImGui STATIC "${rlimgui_SOURCE_DIR}/rlImGui.cpp")
target_include_directories(rlImGui PUBLIC ${rlimgui_SOURCE_DIR})
# This ensures rlImGui knows where to find ImGui headers
target_link_libraries(rlImGui PUBLIC imgui raylib)

# =========================================================

# --- 5. Setup Executable ---
add_executable(LabelForge
    src/main.cpp
    src/utils.cpp
    src/ui.cpp
)

# --- 6. Include Paths ---
target_include_directories(LabelForge PRIVATE 
    include/ 
    ${json_SOURCE_DIR}/include
    # Note: We don't need to add imgui/rlimgui include paths here
    # because we used 'PUBLIC' in the library definitions above.
)

# --- 7. Link Libraries ---
target_link_libraries(LabelForge PRIVATE 
    rlImGui  # Now this refers to our manual target, not a system lib
    imgui    # Same here
    raylib 
    nlohmann_json::nlohmann_json
)

# System specific linking
if (WIN32)
    target_link_libraries(LabelForge PRIVATE winmm)
elseif (UNIX AND NOT APPLE)
    target_link_libraries(LabelForge PRIVATE m pthread dl)
endif()