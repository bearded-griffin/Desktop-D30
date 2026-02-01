cmake_minimum_required(VERSION 3.14)
project(Desktop-D30 VERSION 1.0.0 LANGUAGES CXX)

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

# --- FIND SYSTEM BLUETOOTH LIBRARY ---
find_library(BLUETOOTH_LIB bluetooth REQUIRED)

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
add_executable(Desktop-D30
    src/main.cpp
    src/utils.cpp
    src/ui.cpp
    src/printer.cpp
    src/protocol.cpp
    src/assets.cpp
)

target_include_directories(Desktop-D30 PRIVATE 
    include/ 
    ${json_SOURCE_DIR}/include
    ${pfd_SOURCE_DIR} # Include path for file dialogs
    ${qrcodegen_SOURCE_DIR}/cpp
    qrcodegen
)

target_link_libraries(Desktop-D30 PRIVATE 
    rlImGui 
    imgui 
    raylib 
    nlohmann_json::nlohmann_json
    qrcodegen
    ${BLUETOOTH_LIB}
)

if (WIN32)
    target_link_libraries(Desktop-D30 PRIVATE winmm)
elseif (UNIX AND NOT APPLE)
    target_link_libraries(Desktop-D30 PRIVATE m pthread dl)
endif()

# --- INSTALLATION INSTRUCTIONS ---
# 1. Install the Executable to /opt/Desktop-D30
install(TARGETS Desktop-D30 DESTINATION .)

# 2. Install the Assets folder to /opt/Desktop-D30/assets
install(DIRECTORY assets DESTINATION .)

# 3. Install the Desktop File (for the Start Menu)
#install(FILES Desktop-D30.desktop DESTINATION /usr/share/applications)
install(FILES Desktop-D30.desktop DESTINATION .)

# 4. Install the Icon (for the Start Menu)
#install(FILES assets/app_icon.png DESTINATION /usr/share/pixmaps RENAME Desktop-D30.png)
install(FILES assets/app_icon.png DESTINATION .)



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