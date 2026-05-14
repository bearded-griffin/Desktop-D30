#   This file is part of Desktop-D30
#   Copyright (C) 2026 bearded-griffin
# 
#   This program is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation version 3 of the License.
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#   You should have received a copy of the GNU General Public License
#   along with this program.  If not, see <https://www.gnu.org/licenses/>.

cmake_minimum_required(VERSION 3.14)
project(Desktop-D30 VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} --coverage")

# --- TESTS OPTION ---
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    option(BUILD_TESTING "Build the tests" OFF)
else()
    option(BUILD_TESTING "Build the tests" ON)
endif()

# Define UNIT_TESTING if we are building tests
if(BUILD_TESTING)
    add_compile_definitions(UNIT_TESTING)
endif()


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
# Get Raylib include directory
get_target_property(RAYLIB_INCLUDE_DIR raylib INTERFACE_INCLUDE_DIRECTORIES)

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

# 7. Fetch GoogleTest
message(STATUS "Fetching GoogleTest...")
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG release-1.12.1
    GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(googletest)

# Enable testing
enable_testing()
include(GoogleTest)



# --- FIND SYSTEM BLUETOOTH LIBRARY ---
if(UNIX AND NOT APPLE)
    find_library(BLUETOOTH_LIB bluetooth REQUIRED)
else()
    set(BLUETOOTH_LIB "")
endif()

# --- MANUAL LIBRARY GENERATION ---
file(GLOB IMGUI_SOURCES "${imgui_SOURCE_DIR}/*.cpp")
add_library(imgui STATIC ${IMGUI_SOURCES})
target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR})

add_library(qrcodegen STATIC "${qrcodegen_SOURCE_DIR}/cpp/qrcodegen.cpp")
target_include_directories(qrcodegen PUBLIC "${qrcodegen_SOURCE_DIR}/cpp")

add_library(rlImGui STATIC "${rlimgui_SOURCE_DIR}/rlImGui.cpp")
target_include_directories(rlImGui PUBLIC ${rlimgui_SOURCE_DIR})
target_link_libraries(rlImGui PUBLIC imgui raylib)

# --- INCLUDE DIRECTORIES ---
include_directories(
    ${RAYLIB_INCLUDE_DIR}
    ${imgui_SOURCE_DIR}
    ${rlimgui_SOURCE_DIR}
    ${json_SOURCE_DIR}/include
    ${pfd_SOURCE_DIR}
    ${qrcodegen_SOURCE_DIR}/cpp
    ${CMAKE_SOURCE_DIR}/include
)

# --- MODULE LIBRARIES ---
# Create libraries for each module
add_library(input src/input.cpp)
target_include_directories(input PUBLIC 
    ${CMAKE_SOURCE_DIR}/include
    ${RAYLIB_INCLUDE_DIR}
)

add_library(camera src/camera.cpp)
target_include_directories(camera PUBLIC 
    ${CMAKE_SOURCE_DIR}/include
    ${RAYLIB_INCLUDE_DIR}
)

add_library(rendering src/rendering.cpp)
target_include_directories(rendering PUBLIC 
    ${CMAKE_SOURCE_DIR}/include
    ${RAYLIB_INCLUDE_DIR}
)
target_link_libraries(rendering PUBLIC assets)

add_library(objects src/objects.cpp)
target_include_directories(objects PUBLIC 
    ${CMAKE_SOURCE_DIR}/include
    ${RAYLIB_INCLUDE_DIR}
)
target_link_libraries(objects PUBLIC assets)

add_library(utils src/utils.cpp)
target_include_directories(utils PUBLIC 
    ${CMAKE_SOURCE_DIR}/include
    ${RAYLIB_INCLUDE_DIR}
    ${json_SOURCE_DIR}/include
    ${pfd_SOURCE_DIR} # Include path for file dialogs
    ${qrcodegen_SOURCE_DIR}/cpp
)
target_link_libraries(utils PUBLIC rendering)

add_library(printer src/printer.cpp)
target_include_directories(printer PUBLIC 
    ${CMAKE_SOURCE_DIR}/include
    ${RAYLIB_INCLUDE_DIR}
)
target_link_libraries(printer PUBLIC  ${BLUETOOTH_LIB})
if(WIN32)
    target_link_libraries(printer PUBLIC ws2_32)
endif()

add_library(protocol src/protocol.cpp)
target_include_directories(protocol PUBLIC 
    ${CMAKE_SOURCE_DIR}/include
    ${RAYLIB_INCLUDE_DIR}
)

add_library(ui src/ui.cpp)
target_include_directories(ui PUBLIC 
    ${CMAKE_SOURCE_DIR}/include
    ${RAYLIB_INCLUDE_DIR}
)
target_link_libraries(ui PUBLIC printer protocol)

add_library(assets src/assets.cpp)
target_include_directories(assets PUBLIC 
    ${CMAKE_SOURCE_DIR}/include
    ${RAYLIB_INCLUDE_DIR}
)


# --- EXECUTABLE ---
add_executable(Desktop-D30
    src/main.cpp
)

# Hide the console window on Windows for non-debug builds
set_target_properties(Desktop-D30 PROPERTIES WIN32_EXECUTABLE ON)

target_include_directories(Desktop-D30 PRIVATE 
    include/ 
    ${json_SOURCE_DIR}/include
    ${pfd_SOURCE_DIR} # Include path for file dialogs
    ${qrcodegen_SOURCE_DIR}/cpp
    qrcodegen
    gtest
    gtest_main
)

target_link_libraries(Desktop-D30 PRIVATE
    camera
    input
    rendering
    objects
    utils
    printer
    protocol
    ui
    assets 
    rlImGui 
    imgui 
    raylib 
    nlohmann_json::nlohmann_json
    qrcodegen
    ${BLUETOOTH_LIB}
)

# Copy assets to build directory for easier running during development
add_custom_command(TARGET Desktop-D30 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:Desktop-D30>/assets
    COMMENT "Copying assets to build directory..."
)

if (WIN32)
    target_link_libraries(Desktop-D30 PRIVATE winmm ws2_32)
    # Statically link standard libraries to avoid DLL dependency issues on Windows
    target_link_options(Desktop-D30 PRIVATE "-static-libgcc" "-static-libstdc++" "-static")
elseif (UNIX AND NOT APPLE)
    target_link_libraries(Desktop-D30 PRIVATE m pthread dl)
endif()

# --- TESTS ---
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

# --- INSTALLATION INSTRUCTIONS ---
# 1. Install the Executable to bin
install(TARGETS Desktop-D30 DESTINATION bin)

# 2. Install the Assets folder to bin/assets (relative to exe)
install(DIRECTORY assets DESTINATION bin)

# 3. Install the Desktop File
install(FILES Desktop-D30.desktop DESTINATION share/applications)

# 4. Install the Icon
install(FILES assets/Desktop-D30.png DESTINATION share/icons/hicolor/512x512/apps RENAME Desktop-D30.png)



# --- PACKAGING (CPack) ---
set(CPACK_PACKAGE_NAME "Desktop-D30")
set(CPACK_PACKAGE_VENDOR "BeardedGriffin")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Custom Label Printer Suite")
set(CPACK_PACKAGE_VERSION "1.0.0")
set(CPACK_PACKAGE_CONTACT "bearded.griffinsc@protonmail.com")

# Install to /opt/Desktop-D30 so relative paths work nicely
set(CPACK_PACKAGING_INSTALL_PREFIX "/opt/Desktop-D30")

# Generate a .deb file (Debian/Ubuntu) and a .tar.gz (Generic)
set(CPACK_GENERATOR "DEB;TGZ")

# Debian Specifics
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "BeardedGriffin")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON) # Auto-detect dependencies
set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_CURRENT_SOURCE_DIR}/postinst")

include(CPack)