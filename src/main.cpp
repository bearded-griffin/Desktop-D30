/*!***************************************************
 * @file     main.cpp
 * @brief    The main entry point for Desktop-D30
 * @details  Handels the input from the user and draws the interface.
 * @note     Updated with Camera Centering, Object Clamping, and Deletion.
 * @date     2026.01.20
 * @author   bearded.griffin
 ****************************************************/

#include <algorithm>  // For std::max/min clamps

#include "assets.h"
#include "barcode.h"
#include "imgui.h"
#include "raylib.h"
#include "rlImGui.h"
#include "types.h"
#include "ui.h"
#include "utils.h"

int main() {
  const int screenWidth = 1280;
  const int screenHeight = 800;

  SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
  InitWindow(screenWidth, screenHeight, "Desktop-D30");
  SetTargetFPS(60);

  rlImGuiSetup(true);

  Project currentProject;
  currentProject.objects.push_back({ObjectType::Text, 20, 40, 0, 0,
                                    "Desktop-D30", "", "", 30.0f, 0x000000FF});
  currentProject.isDirty = true;

  // --- FIX 1: CENTER CAMERA INITIALLY ---
  // We want the camera to look at the center of the label, not (0,0)
  LabelSize initialSize = Utils::LabelSizes[currentProject.selectedLabelIndex];

  Camera2D camera = {0};
  camera.zoom = 1.0f;
  camera.offset = {screenWidth / 2.0f,
                   screenHeight / 2.0f};  // Center of screen
  camera.target = {initialSize.width / 2.0f,
                   initialSize.height / 2.0f};  // Center of label

  int selectedIndex = -1;
  bool isDraggingObject = false;
  Vector2 dragOffset = {0, 0};

  // Track if label size changed to re-center camera
  int lastLabelIndex = currentProject.selectedLabelIndex;

  while (!WindowShouldClose() && !UI::ShouldClose()) {
    // --- Window Title ---
    std::string title = "Desktop-D30";
    if (!currentProject.csvFilePath.empty()) {
      title += ": ";
      if (currentProject.isDirty) {
        title += "*";
      }
      // Extract just the filename from the path
      size_t lastSlash = currentProject.csvFilePath.find_last_of("/\\");
      if (lastSlash != std::string::npos) {
        title += currentProject.csvFilePath.substr(lastSlash + 1);
      } else {
        title += currentProject.csvFilePath;
      }
    }
    SetWindowTitle(title.c_str());

    // Handle Resize: Keep camera offset in center of window
    camera.offset = {GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f};

    // Re-center if label size changes
    if (lastLabelIndex != currentProject.selectedLabelIndex) {
      LabelSize sz = Utils::LabelSizes[currentProject.selectedLabelIndex];
      camera.target = {sz.width / 2.0f, sz.height / 2.0f};
      lastLabelIndex = currentProject.selectedLabelIndex;
    }

    Vector2 mouseWorld = GetScreenToWorld2D(GetMousePosition(), camera);
    bool mouseHandledByUI = ImGui::GetIO().WantCaptureMouse;

    // --- INPUT HANDLING ---

    // 1. DELETE OBJECT (Delete or Backspace)
    if (!mouseHandledByUI && selectedIndex != -1) {
      if (IsKeyPressed(KEY_DELETE) || IsKeyPressed(KEY_BACKSPACE)) {
        currentProject.objects.erase(currentProject.objects.begin() +
                                     selectedIndex);
        currentProject.isDirty = true;
        selectedIndex = -1;
        isDraggingObject = false;
      }
    }

    // 2. PANNING (Space + Drag)
    if (IsKeyDown(KEY_SPACE)) {
      if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
        Vector2 delta = Utils::GetMouseDeltaWorld(camera);
        camera.target = Vector2Add(camera.target, delta);
      }
    }
    // 3. SELECTION & DRAGGING
    else if (!mouseHandledByUI) {
      if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        int clickedIndex = -1;
        // Iterate backwards to select top-most item
        for (int i = currentProject.objects.size() - 1; i >= 0; i--) {
          if (CheckCollisionPointRec(
                  mouseWorld,
                  Utils::GetObjectBounds(currentProject.objects[i]))) {
            clickedIndex = i;
            break;
          }
        }
        selectedIndex = clickedIndex;
        if (selectedIndex != -1) {
          isDraggingObject = true;
          dragOffset = {mouseWorld.x - currentProject.objects[selectedIndex].x,
                        mouseWorld.y - currentProject.objects[selectedIndex].y};
        }
      }
      if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) isDraggingObject = false;

      // --- FIX 2: DRAGGING WITH BOUNDS CLAMPING ---
      if (isDraggingObject && selectedIndex != -1) {
        LabelObject &obj = currentProject.objects[selectedIndex];

        // 1. Calculate Proposed Position
        float newX = mouseWorld.x - dragOffset.x;
        float newY = mouseWorld.y - dragOffset.y;

        // 2. Get Object Size (Text, QR, etc.)
        Rectangle bounds = Utils::GetObjectBounds(obj);
        LabelSize canvasSz =
            Utils::LabelSizes[currentProject.selectedLabelIndex];

        // 3. Clamp X
        // Prevent going left of 0
        if (newX < 0) newX = 0;
        // Prevent going right of canvas edge (CanvasWidth - ObjectWidth)
        else if (newX + bounds.width > canvasSz.width)
          newX = canvasSz.width - bounds.width;

        // 4. Clamp Y
        // Prevent going above 0
        if (newY < 0) newY = 0;
        // Prevent going below canvas edge
        else if (newY + bounds.height > canvasSz.height)
          newY = canvasSz.height - bounds.height;

        // 5. Apply
        obj.x = newX;
        obj.y = newY;
        currentProject.isDirty = true;
      }
    }

    // 4. ZOOMING
    float wheel = GetMouseWheelMove();
    if (wheel != 0 && !mouseHandledByUI) {
      camera.zoom += wheel * 0.1f;
      if (camera.zoom < 0.1f) camera.zoom = 0.1f;
    }

    // --- DRAWING ---
    BeginDrawing();

    ClearBackground(currentProject.darkTheme ? Color{40, 40, 40, 255}
                                             : RAYWHITE);

    BeginMode2D(camera);
    LabelSize currentSize =
        Utils::LabelSizes[currentProject.selectedLabelIndex];

    // Draw Canvas Area (Centered white box)
    DrawRectangle(0, 0, (int)currentSize.width, (int)currentSize.height, WHITE);
    DrawRectangleLines(0, 0, (int)currentSize.width, (int)currentSize.height,
                       GRAY);

    // Grid Rendering
    if (currentProject.showGrid) {
      const int gridSize = 20;
      for (int x = 0; x <= currentSize.width; x += gridSize)
        DrawLine(x, 0, x, currentSize.height, LIGHTGRAY);
      for (int y = 0; y <= currentSize.height; y += gridSize)
        DrawLine(0, y, currentSize.width, y, LIGHTGRAY);
    }

    // Object Rendering
    for (int i = 0; i < currentProject.objects.size(); i++) {
      auto &obj = currentProject.objects[i];
      Color col = GetColor(obj.colorHex);

      if (obj.type == ObjectType::Text || obj.type == ObjectType::Field) {
        // LOOKUP FONT
        Font displayFont = AssetManager::Get().GetFont(obj.fontName);
        Utils::DrawTextBox(nullptr, displayFont, obj.data.c_str(), obj.x, obj.y,
                           obj.fontSize, 2.0f, col, obj.width);
        // Visualize word wrap box
        if (i == selectedIndex && obj.width > 0) {
          DrawRectangleLines(obj.x, obj.y, obj.width,
                             obj.height > 0 ? obj.height : obj.fontSize * 2,
                             Fade(SKYBLUE, 0.5f));
        }
        // Use our shared helper for consistent rendering
        // nullptr = Draw to Screen
        Utils::DrawTextBox(nullptr, displayFont, obj.data.c_str(), obj.x, obj.y,
                           obj.fontSize, 2.0f, col, obj.width);

      } else if (obj.type == ObjectType::QRCode) {
        Utils::DrawQRCode(obj.data, obj.x, obj.y, obj.width, col);
        // Draw faint border if not selected so we can find it if white
        DrawRectangleLines(obj.x, obj.y, obj.width, obj.width,
                           Fade(GRAY, 0.3f));
      } else if (obj.type == ObjectType::Image) {
        // 1. Lazy Load: If path exists but texture doesn't, load it.
        if (obj.texture.id == 0 && !obj.data.empty() &&
            FileExists(obj.data.c_str())) {
          Image img = LoadImage(obj.data.c_str());
          obj.texture = LoadTextureFromImage(img);
          UnloadImage(img);
          // Auto-size if new
          if (obj.width == 0) obj.width = (float)obj.texture.width;
          if (obj.height == 0) obj.height = (float)obj.texture.height;
        }

        // 2. Draw
        if (obj.texture.id != 0) {
          // DrawTexturePro allows sizing
          Rectangle src = {0, 0, (float)obj.texture.width,
                           (float)obj.texture.height};
          Rectangle dst = {obj.x, obj.y, obj.width, obj.height};
          DrawTexturePro(obj.texture, src, dst, {0, 0}, 0.0f, WHITE);
        } else {
          // Placeholder
          DrawRectangleLines(obj.x, obj.y, obj.width, obj.height, BLACK);
          DrawText("IMG", obj.x + 5, obj.y + 5, 10, BLACK);
        }

        // Draw Border
        DrawRectangleLines(obj.x, obj.y, obj.width, obj.height, BLACK);
      } else if (obj.type == ObjectType::Line) {
        Vector2 start = {obj.x, obj.y};
        // In editor, Width/Height control the endpoint relative to X/Y
        Vector2 end = {obj.x + obj.width, obj.y + obj.height};

        DrawLineEx(start, end, obj.fontSize, col);

        // Draw selection handles for line endpoints if selected
        if (i == selectedIndex) {
          DrawCircleV(start, 4, SKYBLUE);
          DrawCircleV(end, 4, SKYBLUE);
        }
      } else if (obj.type == ObjectType::ShapeRect ||
                 obj.type == ObjectType::Border) {
        Rectangle rec = {obj.x, obj.y, obj.width, obj.height};

        // 1. Calculate Roundness
        float minDim = (rec.width < rec.height) ? rec.width : rec.height;
        float roundness = 0.0f;
        if (minDim > 0) roundness = obj.cornerRadius / (minDim / 2.0f);
        if (roundness > 1.0f) roundness = 1.0f;
        if (roundness < 0.0f) roundness = 0.0f;

        // 2. Draw Outer Box (The Color/Black)
        // We use DrawRectangleRounded (Filled) which exists in ALL Raylib
        // versions
        DrawRectangleRounded(rec, roundness, 10, col);

        // 3. Draw Inner Box (The White "Hole") to simulate thickness
        float thick = obj.fontSize;

        // Only draw the hole if the border isn't thicker than the object itself
        if (thick * 2 < rec.width && thick * 2 < rec.height) {
          Rectangle inner = {rec.x + thick, rec.y + thick,
                             rec.width - (thick * 2), rec.height - (thick * 2)};

          // Recalculate inner roundness so corners look concentric
          float innerRadius =
              (obj.cornerRadius > thick) ? obj.cornerRadius - thick : 0;
          float innerMin =
              (inner.width < inner.height) ? inner.width : inner.height;
          float innerRoundness =
              (innerMin > 0) ? innerRadius / (innerMin / 2.0f) : 0;

          // Draw the white center (Matches the label background)
          DrawRectangleRounded(inner, innerRoundness, 10, WHITE);
        }
      } else if (obj.type == ObjectType::ShapeCircle) {
        float radius = obj.width / 2.0f;
        Vector2 center = {obj.x + radius, obj.y + radius};
        // DrawRing allows thickness: center, innerRadius, outerRadius,
        // startAngle, endAngle, segments, color
        DrawRing(center, radius - obj.fontSize, radius, 0, 360, 0, col);
      } else if (obj.type == ObjectType::Barcode) {
        std::string code = Barcode::Encode128(obj.data);
        float moduleWidth = obj.width / (float)code.length();

        for (int i = 0; i < code.length(); i++) {
          if (code[i] == '1') {
            DrawRectangle((int)(obj.x + (i * moduleWidth)), (int)obj.y,
                          (int)(moduleWidth + 1.0f),  // +1 for screen crispness
                          (int)obj.height, col);
          }
        }
        DrawRectangleLines(obj.x, obj.y, obj.width, obj.height,
                           Fade(GRAY, 0.5f));
      }

      // Selection Box (Skip for Line as it has its own handles)
      if (i == selectedIndex && obj.type != ObjectType::Line) {
        Rectangle bounds = Utils::GetObjectBounds(obj);
        DrawRectangleLinesEx(
            {bounds.x - 5, bounds.y - 5, bounds.width + 10, bounds.height + 10},
            2.0f / camera.zoom, SKYBLUE);
      }
    }
    EndMode2D();

    rlImGuiBegin();
    UI::DrawMainMenu(currentProject);
    UI::DrawSidebar(currentProject, selectedIndex);
    UI::DrawExitConfirmation(currentProject);
    UI::DrawLoadConfirmation(currentProject);
    rlImGuiEnd();

    EndDrawing();
  }

  // Cleanup Textures
  for (auto &obj : currentProject.objects) {
    if (obj.texture.id != 0) {
      UnloadTexture(obj.texture);
    }
  }

  rlImGuiShutdown();
  CloseWindow();
  return 0;
}