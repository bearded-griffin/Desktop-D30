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
    camera.offset = { screenWidth / 2.0f, screenHeight / 2.0f };

    Project currentProject;
    currentProject.objects.push_back({ ObjectType::Text, 20, 40, 0, 0, "LabelForge", 30.0f, 0x000000FF });

    int selectedIndex = -1;
    bool isDraggingObject = false;
    Vector2 dragOffset = { 0, 0 };

    while (!WindowShouldClose()) {
        Vector2 mouseWorld = GetScreenToWorld2D(GetMousePosition(), camera);
        bool mouseHandledByUI = ImGui::GetIO().WantCaptureMouse;

        // Input Handling (same as before, updated for struct changes)
        if (IsKeyDown(KEY_SPACE) && IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
             camera.target = Vector2Add(camera.target, Utils::GetMouseDeltaWorld(camera));
        } else if (!mouseHandledByUI) {
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                int clickedIndex = -1;
                for (int i = currentProject.objects.size() - 1; i >= 0; i--) {
                    if (CheckCollisionPointRec(mouseWorld, Utils::GetObjectBounds(currentProject.objects[i]))) {
                        clickedIndex = i; break;
                    }
                }
                selectedIndex = clickedIndex;
                if (selectedIndex != -1) {
                    isDraggingObject = true;
                    dragOffset = { mouseWorld.x - currentProject.objects[selectedIndex].x, mouseWorld.y - currentProject.objects[selectedIndex].y };
                }
            }
            if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) isDraggingObject = false;
            if (isDraggingObject && selectedIndex != -1) {
                currentProject.objects[selectedIndex].x = mouseWorld.x - dragOffset.x;
                currentProject.objects[selectedIndex].y = mouseWorld.y - dragOffset.y;
            }
        }
        float wheel = GetMouseWheelMove();
        if (wheel != 0 && !mouseHandledByUI) {
             camera.zoom += wheel * 0.1f;
             if (camera.zoom < 0.1f) camera.zoom = 0.1f;
        }

        BeginDrawing();
        
        // --- 1. Dark Mode Background ---
        ClearBackground(currentProject.darkTheme ? Color{40, 40, 40, 255} : RAYWHITE);

        BeginMode2D(camera);
            LabelSize currentSize = Utils::LabelSizes[currentProject.selectedLabelIndex];
            
            // Draw Canvas Area
            DrawRectangle(0, 0, (int)currentSize.width, (int)currentSize.height, WHITE);
            DrawRectangleLines(0, 0, (int)currentSize.width, (int)currentSize.height, GRAY);

            // --- 2. Grid Rendering ---
            if (currentProject.showGrid) {
                const int gridSize = 20;
                // Vertical lines
                for (int x = 0; x <= currentSize.width; x += gridSize)
                    DrawLine(x, 0, x, currentSize.height, LIGHTGRAY);
                // Horizontal lines
                for (int y = 0; y <= currentSize.height; y += gridSize)
                    DrawLine(0, y, currentSize.width, y, LIGHTGRAY);
            }

            // --- 3. Object Rendering ---
           for (int i = 0; i < currentProject.objects.size(); i++) {
                auto& obj = currentProject.objects[i];
                Color col = GetColor(obj.colorHex); 
                
                if (obj.type == ObjectType::Text) {
                    DrawTextEx(GetFontDefault(), obj.data.c_str(), {obj.x, obj.y}, obj.fontSize, 2.0f, col);
                } 
                else if (obj.type == ObjectType::Field) {
                    DrawTextEx(GetFontDefault(), obj.data.c_str(), {obj.x, obj.y}, obj.fontSize, 2.0f, BLUE);
                }
                else if (obj.type == ObjectType::QRCode) {
                    // --- NEW: Real QR Generation ---
                    // We pass the Width as the "Size" (assuming square for now)
                    Utils::DrawQRCode(obj.data, obj.x, obj.y, obj.width, col);
                    
                    // Draw a border if selected so we can see bounds even if white
                    DrawRectangleLines(obj.x, obj.y, obj.width, obj.width, Fade(GRAY, 0.5f));
                }
                else if (obj.type == ObjectType::Image) {
                    DrawRectangleLines(obj.x, obj.y, obj.width, obj.height, BLACK);
                    DrawText("IMG", obj.x + 5, obj.y + 5, 10, BLACK);
                }
                
                if (i == selectedIndex) {
                    Rectangle bounds = Utils::GetObjectBounds(obj);
                    DrawRectangleLinesEx({bounds.x - 5, bounds.y - 5, bounds.width + 10, bounds.height + 10}, 2.0f/camera.zoom, SKYBLUE);
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