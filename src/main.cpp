#include "raylib.h"
#include "rlImGui.h"
#include "imgui.h"
#include "types.h"
#include "utils.h"
#include "ui.h"

int main() {
    const int screenWidth = 1280;
    const int screenHeight = 800;
    
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(screenWidth, screenHeight, "LabelForge");
    SetTargetFPS(60);

    rlImGuiSetup(true);

    Camera2D camera = { 0 };
    camera.zoom = 1.0f;
    camera.offset = { screenWidth / 2.0f, screenHeight / 2.0f }; // Center camera

    Project currentProject;
    currentProject.objects.push_back({ 20.0f, 40.0f, "LabelForge", 30.0f, 0x000000FF });

    int selectedIndex = -1;
    
    // dragging state
    bool isDraggingObject = false;
    Vector2 dragOffset = { 0, 0 };

    while (!WindowShouldClose()) {
        
        // 1. Calculate World Mouse Position
        Vector2 mouseWorld = GetScreenToWorld2D(GetMousePosition(), camera);
        bool mouseHandledByUI = ImGui::GetIO().WantCaptureMouse;

        // --- INPUT HANDLING ---
        
        // A. Camera Pan (Space + Drag)
        if (IsKeyDown(KEY_SPACE)) {
            if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
                Vector2 delta = Utils::GetMouseDeltaWorld(camera);
                camera.target = Vector2Add(camera.target, delta);
            }
        }
        // B. Object Selection & Dragging (No Space)
        else if (!mouseHandledByUI) {
            
            // Mouse Down: Check for hit
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                // Check if we clicked an object (Iterate backwards to select top-most)
                int clickedIndex = -1;
                for (int i = currentProject.objects.size() - 1; i >= 0; i--) {
                    Rectangle bounds = Utils::GetObjectBounds(currentProject.objects[i]);
                    if (CheckCollisionPointRec(mouseWorld, bounds)) {
                        clickedIndex = i;
                        break;
                    }
                }

                selectedIndex = clickedIndex;

                if (selectedIndex != -1) {
                    isDraggingObject = true;
                    // Calculate offset so the object doesn't "snap" to the mouse center
                    LabelObject& obj = currentProject.objects[selectedIndex];
                    dragOffset = { mouseWorld.x - obj.x, mouseWorld.y - obj.y };
                }
            }

            // Mouse Up: Stop dragging
            if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
                isDraggingObject = false;
            }

            // Mouse Dragging: Update position
            if (isDraggingObject && selectedIndex != -1 && IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
                LabelObject& obj = currentProject.objects[selectedIndex];
                obj.x = mouseWorld.x - dragOffset.x;
                obj.y = mouseWorld.y - dragOffset.y;
            }
        }

        // C. Zoom
        float wheel = GetMouseWheelMove();
        if (wheel != 0 && !mouseHandledByUI) {
            camera.zoom += wheel * 0.1f;
            if (camera.zoom < 0.1f) camera.zoom = 0.1f;
        }


        // --- DRAWING ---
        BeginDrawing();
        ClearBackground(RAYWHITE); // App Background

        BeginMode2D(camera);
            
            // 1. Draw Canvas (The Label Background)
            // Get dimensions from the selected label size
            LabelSize currentSize = Utils::LabelSizes[currentProject.selectedLabelIndex];
            
            // Draw White Label Background
            DrawRectangle(0, 0, (int)currentSize.width, (int)currentSize.height, WHITE);
            // Draw Label Border
            DrawRectangleLines(0, 0, (int)currentSize.width, (int)currentSize.height, GRAY);
            
            // Draw Canvas Shadow (Optional, for aesthetics)
            DrawLine(currentSize.width + 2, 2, currentSize.width + 2, currentSize.height + 2, Fade(GRAY, 0.5f));
            DrawLine(2, currentSize.height + 2, currentSize.width + 2, currentSize.height + 2, Fade(GRAY, 0.5f));


            // 2. Draw Objects
            for (int i = 0; i < currentProject.objects.size(); i++) {
                auto& obj = currentProject.objects[i];
                Color col = GetColor(obj.colorHex); 
                
                DrawTextEx(GetFontDefault(), obj.text.c_str(), {obj.x, obj.y}, obj.fontSize, 2.0f, col);
                
                // Draw Selection Box
                if (i == selectedIndex) {
                    Rectangle bounds = Utils::GetObjectBounds(obj);
                    // Draw a blue border around selected item
                    DrawRectangleLinesEx({bounds.x - 5, bounds.y - 5, bounds.width + 10, bounds.height + 10}, 2.0f/camera.zoom, BLUE);
                }
            }

        EndMode2D();

        rlImGuiBegin();
            UI::DrawMainMenu(currentProject); 
            UI::DrawSidebar(currentProject, selectedIndex);
        rlImGuiEnd();

        EndDrawing();
    }

    rlImGuiShutdown();
    CloseWindow();
    return 0;
}