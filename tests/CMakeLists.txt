# Enable testing
enable_testing()

# Collect all test files
file(GLOB_RECURSE TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

# Check if we found any test files
if(NOT TEST_SOURCES)
    message(WARNING "No test source files found in ${CMAKE_CURRENT_SOURCE_DIR}")
else()
    message(STATUS "Found test files: ${TEST_SOURCES}")
endif()

# Only create test executable if we have test sources
if(TEST_SOURCES)
  include_directories(../include)
    # Create test executable
    add_executable(run_tests ${TEST_SOURCES})

    # Link libraries
    target_link_libraries(run_tests
        GTest::gtest
        GTest::gmock
        camera
        input
        utils
        rendering
        objects
        assets
        protocol
        printer # protocol depends on printer
        ui
        raylib
        imgui
        rlImGui
        nlohmann_json::nlohmann_json
        qrcodegen
        ${BLUETOOTH_LIB}
    )

    # Add linker flags to wrap Raylib's GetScreenWidth/Height functions
    target_link_options(run_tests PRIVATE 
        "SHELL:-Wl,-wrap,GetScreenWidth,-wrap,GetScreenHeight"
        "SHELL:-Wl,-wrap,ExportImage"
        "SHELL:-Wl,-wrap,FileExists"
        "SHELL:-Wl,-wrap,GetMouseDelta,-wrap,GetMousePosition,-wrap,GetMouseWheelMove"
        "SHELL:-Wl,-wrap,IsKeyDown,-wrap,IsKeyPressed,-wrap,IsMouseButtonPressed,-wrap,IsMouseButtonDown,-wrap,IsMouseButtonReleased"
        "SHELL:-Wl,-wrap,SetMouseCursor,-wrap,SetWindowTitle"
        "SHELL:-Wl,-wrap,GetScreenToWorld2D,-wrap,Vector2Add,-wrap,Vector2Scale"
        "SHELL:-Wl,-wrap,CheckCollisionPointRec,-wrap,CheckCollisionPointCircle"
        "SHELL:-Wl,-wrap,MeasureTextEx"
        "SHELL:-Wl,-wrap,DrawTextEx,-wrap,DrawText"
        "SHELL:-Wl,-wrap,DrawLineEx,-wrap,DrawLine"
        "SHELL:-Wl,-wrap,DrawRectangleRounded,-wrap,DrawRing"
        "SHELL:-Wl,-wrap,DrawRectangle,-wrap,DrawRectangleRec,-wrap,DrawRectangleLines,-wrap,DrawRectangleLinesEx"
        "SHELL:-Wl,-wrap,DrawCircleV,-wrap,DrawCircleLinesV"
        "SHELL:-Wl,-wrap,DrawTexturePro"
        "SHELL:-Wl,-wrap,GetColor,-wrap,Fade"
        "SHELL:-Wl,-wrap,BeginDrawing,-wrap,EndDrawing,-wrap,BeginMode2D,-wrap,EndMode2D,-wrap,ClearBackground"
        "SHELL:-Wl,-wrap,GenImageColor,-wrap,ImageDrawRectangle,-wrap,ImageDraw,-wrap,ImageDrawTextEx,-wrap,ImageResize,-wrap,ImageFormat"
        "SHELL:-Wl,-wrap,ImageDrawRectangleLines,-wrap,ImageDrawLineEx,-wrap,ImageDrawCircle"
        "SHELL:-Wl,-wrap,UnloadImage,-wrap,LoadImage,-wrap,LoadTextureFromImage,-wrap,UnloadTexture"
    )

    # Add test
    add_test(NAME AllTests COMMAND run_tests)
endif()
