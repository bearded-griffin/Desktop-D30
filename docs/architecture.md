# Architecture Guide

This document provides a high-level technical overview of how Desktop-D30 is structured. It is intended for developers who want to understand the "soul" of the application beyond individual function definitions.

## 1. Core Data Model (`types.h`)

The application follows a simple, flat data model.

*   **`LabelObject`**: The atom of the application. It contains position, size, rotation, and type-specific data (text, image paths, etc.). 
    *   *Key Feature*: It stores its own `texture` handle for images/barcodes to ensure they aren't re-generated every frame.
*   **`Project`**: A container for a collection of `LabelObject`s, the selected label size, and CSV data.
*   **`InteractionState`**: Manages the "Editor" state, including the selection list, clipboard, and the Undo/Redo stacks.

## 2. The Rendering Pipeline (`rendering.cpp`)

There are two distinct rendering paths in Desktop-D30:

### A. Real-time Canvas (UI)
1.  The editor uses **Raylib's Camera2D** to provide zooming and panning.
2.  Each object is rendered via `RENDERING::RenderObject`.
3.  **Rotation Logic**: Objects are rotated around their top-left `(x, y)` coordinate using standard 2D rotation matrices. UI interaction (clicking/dragging) uses an inverse rotation matrix to map mouse coordinates back into the object's local space for accurate collision detection.

### B. Print Generation (Thermal)
1.  **Buffer Allocation**: The project is rendered to an off-screen `Image` buffer matching the physical dimensions of the label (e.g., 240x96 pixels).
2.  **Thresholding**: The color image is converted to grayscale and then to 1-bit black/white using a weighted threshold (0.299R, 0.587G, 0.114B). 
3.  **Rasterization**: The 1-bit pixels are packed into rows where each byte represents 8 horizontal pixels, conforming to the Phomemo D30's RFCOMM protocol.

## 3. Input & History (`input.cpp`)

The interaction system is designed for both mouse precision and keyboard speed.

*   **Undo/Redo**: The system uses a "State Snapshot" approach. Before any destructive or significant change, the entire `Project` object is serialized and pushed onto the `undoStack` (limited to 50 steps). This is inefficient for memory but extremely robust for complex layout changes.
*   **Snapping**: The `HandleObjectDrag` function implements a dual-mode snapping system:
    *   **Grid Snap**: Rounds coordinates to the nearest 20-unit increment.
    *   **Object Snap**: Scans all other objects and "snaps" if edges or centers are within a 5-unit threshold.

## 4. Cross-Platform Bluetooth (`protocol.cpp` / `printer.cpp`)

The Bluetooth layer is abstracted to handle the vast differences between Linux and Windows drivers.

*   **Linux (BlueZ)**: Uses standard POSIX socket calls (`socket`, `connect`, `write`) with `AF_BLUETOOTH` and `BTPROTO_RFCOMM`.
*   **Windows (Winsock2)**: Uses the `BTHPROTO_RFCOMM` protocol. It requires initializing the Winsock library (`WSAStartup`) and uses `send` instead of `write`.
*   **Protocol Layer**: The `Protocol` namespace handles the higher-level "D30 Language"—specifically the header packets, row-by-row image data, and the "End of Print" commands.

## 5. Asset Management (`assets.cpp`)

To keep the UI snappy, assets are managed through a single `AssetManager` singleton.

*   **Lazy Loading**: Fonts and icon thumbnails are only loaded into VRAM when they are first needed or when the library is opened.
*   **Background Load Queue**: During the splash screen, the app initializes a queue of all icon paths. It processes 10 icons per frame during the main loop to prevent the UI from freezing while loading hundreds of tiny textures.

## 6. Project Serialization

Projects are saved as `.d30` files, which are standard JSON. The application uses the `nlohmann::json` library to automate the conversion between C++ structs and JSON text. This allows users to easily inspect or even manually edit project files if needed.
