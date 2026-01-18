# Makefile wrapper for CMake

# Default build type
TYPE ?= Debug

# Standard CMake build folder
BUILD_DIR = build

.PHONY: all build clean run re

# Default target: build the project
all: build

# Configure CMake (Using Ninja)
configure:
	cmake -S . -B $(BUILD_DIR) -G Ninja -DCMAKE_BUILD_TYPE=$(TYPE)

# Compile the project
build:
	@if [ ! -d "$(BUILD_DIR)" ]; then $(MAKE) configure; fi
	cmake --build $(BUILD_DIR)

# Run the executable
run: build
	./$(BUILD_DIR)/LabelForge

# Nuke the build folder
clean:
	rm -rf $(BUILD_DIR)

# "Re-make": Clean and then build
re: clean build