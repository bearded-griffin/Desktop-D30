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
    // Fix: Match new struct format {x, y, text, size, color}
    currentProject.objects.push_back({ 0.0f, 0.0f, "LabelForge", 40.0f, 0x000000FF });

    int selectedIndex = -1;

    while (!WindowShouldClose()) {
        
        // --- INPUT LOGIC ---
        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) && IsKeyDown(KEY_SPACE)) {
             Vector2 delta = Utils::GetMouseDeltaWorld(camera);
             camera.target = Vector2Add(camera.target, delta);
        }
        float wheel = GetMouseWheelMove();
        if (wheel != 0 && !ImGui::GetIO().WantCaptureMouse) {
             camera.zoom += wheel * 0.1f;
             if (camera.zoom < 0.1f) camera.zoom = 0.1f;
        }

        // --- DRAWING ---
        BeginDrawing();
        ClearBackground(RAYWHITE);

        BeginMode2D(camera);
            DrawRectangle(-200, -100, 400, 200, WHITE);
            DrawRectangleLines(-200, -100, 400, 200, LIGHTGRAY);
            
            for (int i = 0; i < currentProject.objects.size(); i++) {
                auto& obj = currentProject.objects[i];
                
                // Fix: Reconstruct Vector2 and Color from flat values
                Color col = GetColor(obj.colorHex); 
                Vector2 pos = { obj.x, obj.y };
                
                DrawTextEx(GetFontDefault(), obj.text.c_str(), pos, obj.fontSize, 2.0f, col);
                
                if (i == selectedIndex) {
                    Vector2 size = MeasureTextEx(GetFontDefault(), obj.text.c_str(), obj.fontSize, 2.0f);
                    DrawRectangleLinesEx({pos.x - 5, pos.y - 5, size.x + 10, size.y + 10}, 2.0f, BLUE);
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