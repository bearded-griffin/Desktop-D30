# Enable testing
enable_testing()

# Collect all test files
# Collect all test files - use absolute path to ensure we find them
file(GLOB_RECURSE TEST_SOURCES 
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)
# set(TEST_SOURCES
#     tests/camera_tests.cpp
# )

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
        utils
        rendering
        objects
        assets
        protocol
        printer # protocol depends on printer
        raylib
        imgui
        rlImGui
        nlohmann_json::nlohmann_json
        qrcodegen
        ${BLUETOOTH_LIB}
    )

    # Add linker flags to wrap Raylib's GetScreenWidth/Height functions
    target_link_options(run_tests PRIVATE "SHELL:-Wl,-wrap,GetScreenWidth,-wrap,GetScreenHeight")

    # Add test
    add_test(NAME AllTests COMMAND run_tests)
endif()
