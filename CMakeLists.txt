cmake_minimum_required(VERSION 3.14)
project(LabelForge VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# --- DEBUG FLAGS ---
set(CMAKE_VERBOSE_MAKEFILE ON)
set(FETCHCONTENT_QUIET FALSE)

include(FetchContent)

# 1. Raylib
message(STATUS "Fetching Raylib...")
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG master
    GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(raylib)

# 2. Dear ImGui
message(STATUS "Fetching Dear ImGui...")
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG docking
    GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(imgui)

# 3. rlImGui
message(STATUS "Fetching rlImGui...")
FetchContent_Declare(
    rlimgui
    GIT_REPOSITORY https://github.com/raylib-extras/rlImGui.git
    GIT_TAG main
    GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(rlimgui)

# 4. nlohmann/json
message(STATUS "Fetching nlohmann/json...")
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.2
    GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(json)

# 5. Portable File Dialogs (Header Only - easy fetch)
message(STATUS "Fetching Portable File Dialogs...")
FetchContent_Declare(
    pfd
    GIT_REPOSITORY https://github.com/samhocevar/portable-file-dialogs.git
    GIT_TAG main
    GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(pfd)

# 6. Fetch QR Code Generator (Nayuki)
message(STATUS "Fetching QR Code Generator...")
FetchContent_Declare(
    qrcodegen
    GIT_REPOSITORY https://github.com/nayuki/QR-Code-generator.git
    GIT_TAG master
    GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(qrcodegen)

# --- MANUAL LIBRARY GENERATION ---
file(GLOB IMGUI_SOURCES "${imgui_SOURCE_DIR}/*.cpp")
add_library(imgui STATIC ${IMGUI_SOURCES})
target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR})

add_library(qrcodegen STATIC "${qrcodegen_SOURCE_DIR}/cpp/qrcodegen.cpp")
target_include_directories(qrcodegen PUBLIC "${qrcodegen_SOURCE_DIR}/cpp")

add_library(rlImGui STATIC "${rlimgui_SOURCE_DIR}/rlImGui.cpp")
target_include_directories(rlImGui PUBLIC ${rlimgui_SOURCE_DIR})
target_link_libraries(rlImGui PUBLIC imgui raylib)

# --- EXECUTABLE ---
add_executable(LabelForge
    src/main.cpp
    src/utils.cpp
    src/ui.cpp
)

target_include_directories(LabelForge PRIVATE 
    include/ 
    ${json_SOURCE_DIR}/include
    ${pfd_SOURCE_DIR} # Include path for file dialogs
    ${qrcodegen_SOURCE_DIR}/cpp
    qrcodegen
)

target_link_libraries(LabelForge PRIVATE 
    rlImGui 
    imgui 
    raylib 
    nlohmann_json::nlohmann_json
    qrcodegen
)

if (WIN32)
    target_link_libraries(LabelForge PRIVATE winmm)
elseif (UNIX AND NOT APPLE)
    target_link_libraries(LabelForge PRIVATE m pthread dl)
endif()