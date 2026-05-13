#include "raylib.h"
#include <string>
#include <vector>
#include <filesystem>
#include <map>

namespace fs = std::filesystem;

// --- Mock State Variables ---
static Vector2 g_mockMouseDelta = {0, 0};
static Vector2 g_mockMousePosition = {0, 0};
static float g_mockMouseWheel = 0;
static bool g_mockShiftDown = false;
static bool g_mockSpaceDown = false;
static std::map<int, bool> g_mockKeysPressed;
static std::map<int, bool> g_mockKeysDown;
static std::map<int, bool> g_mockMouseButtonsPressed;
static std::map<int, bool> g_mockMouseButtonsDown;
static std::map<int, bool> g_mockMouseButtonsReleased;
static std::string g_lastWindowTitle = "";

// Rendering Mocks State
static int g_DrawTextExCount = 0;
static std::string g_lastDrawTextExString = "";
static Vector2 g_lastDrawTextExPos = {0,0};

static int g_DrawTextProCount = 0;
static std::string g_lastDrawTextProString = "";
static Vector2 g_lastDrawTextProPos = {0,0};

static int g_DrawLineExCount = 0;
static Vector2 g_lastDrawLineExStart = {0,0};
static Vector2 g_lastDrawLineExEnd = {0,0};

static int g_DrawRectangleRoundedCount = 0;
static Rectangle g_lastDrawRectangleRoundedRec = {0,0,0,0};

static int g_DrawRingCount = 0;
static Vector2 g_lastDrawRingCenter = {0,0};
static float g_lastDrawRingRadiusOuter = 0;

static int g_DrawRectangleCount = 0;
static Rectangle g_lastDrawRectangleRec = {0,0,0,0};

static int g_DrawRectangleLinesCount = 0;
static int g_DrawCircleCount = 0;
static int g_DrawTextureProCount = 0;

// --- Helper Functions to Reset/Check State ---
void ResetMockState() {
    g_mockMouseDelta = {0, 0};
    g_mockMousePosition = {0, 0};
    g_mockMouseWheel = 0;
    g_mockShiftDown = false;
    g_mockSpaceDown = false;
    g_mockKeysPressed.clear();
    g_mockKeysDown.clear();
    g_mockMouseButtonsPressed.clear();
    g_mockMouseButtonsDown.clear();
    g_mockMouseButtonsReleased.clear();
    g_lastWindowTitle = "";
    
    g_DrawTextExCount = 0;
    g_lastDrawTextExString = "";
    g_lastDrawTextExPos = {0,0};

    g_DrawTextProCount = 0;
    g_lastDrawTextProString = "";
    g_lastDrawTextProPos = {0,0};
    
    g_DrawLineExCount = 0;
    g_lastDrawLineExStart = {0,0};
    g_lastDrawLineExEnd = {0,0};
    
    g_DrawRectangleRoundedCount = 0;
    g_lastDrawRectangleRoundedRec = {0,0,0,0};
    
    g_DrawRingCount = 0;
    g_lastDrawRingCenter = {0,0};
    g_lastDrawRingRadiusOuter = 0;
    
    g_DrawRectangleCount = 0;
    g_lastDrawRectangleRec = {0,0,0,0};
    
    g_DrawRectangleLinesCount = 0;
    g_DrawCircleCount = 0;
    g_DrawTextureProCount = 0;
}

void SetMockMouseDelta(Vector2 delta) { g_mockMouseDelta = delta; }
void SetMockMousePosition(Vector2 pos) { g_mockMousePosition = pos; }
void SetMockMouseWheel(float wheel) { g_mockMouseWheel = wheel; }
void SetMockShiftDown(bool down) { g_mockShiftDown = down; }
void SetMockSpaceDown(bool down) { g_mockSpaceDown = down; }
void SetMockKeyPressed(int key, bool pressed) { g_mockKeysPressed[key] = pressed; }
void SetMockKeyDown(int key, bool down) { g_mockKeysDown[key] = down; }
void SetMockMouseButtonPressed(int button, bool pressed) { g_mockMouseButtonsPressed[button] = pressed; }
void SetMockMouseButtonDown(int button, bool down) { g_mockMouseButtonsDown[button] = down; }
void SetMockMouseButtonReleased(int button, bool released) { g_mockMouseButtonsReleased[button] = released; }

std::string GetLastWindowTitle() { return g_lastWindowTitle; }

// Rendering Accessors
int GetDrawTextExCount() { return g_DrawTextExCount; }
std::string GetLastDrawTextExString() { return g_lastDrawTextExString; }
Vector2 GetLastDrawTextExPos() { return g_lastDrawTextExPos; }

int GetDrawTextProCount() { return g_DrawTextProCount; }
std::string GetLastDrawTextProString() { return g_lastDrawTextProString; }
Vector2 GetLastDrawTextProPos() { return g_lastDrawTextProPos; }

int GetDrawLineExCount() { return g_DrawLineExCount; }
Vector2 GetLastDrawLineExStart() { return g_lastDrawLineExStart; }
Vector2 GetLastDrawLineExEnd() { return g_lastDrawLineExEnd; }

int GetDrawRectangleRoundedCount() { return g_DrawRectangleRoundedCount; }
Rectangle GetLastDrawRectangleRoundedRec() { return g_lastDrawRectangleRoundedRec; }

int GetDrawRingCount() { return g_DrawRingCount; }
Vector2 GetLastDrawRingCenter() { return g_lastDrawRingCenter; }
float GetLastDrawRingRadiusOuter() { return g_lastDrawRingRadiusOuter; }

int GetDrawRectangleCount() { return g_DrawRectangleCount; }
int GetDrawRectangleLinesCount() { return g_DrawRectangleLinesCount; }
int GetDrawCircleCount() { return g_DrawCircleCount; }
int GetDrawTextureProCount() { return g_DrawTextureProCount; }


extern "C" {
    // --- Input / Window Wraps ---
    Vector2 __wrap_GetMouseDelta(void) { return g_mockMouseDelta; }
    Vector2 __wrap_GetMousePosition(void) { return g_mockMousePosition; }
    float __wrap_GetMouseWheelMove(void) { return g_mockMouseWheel; }

    bool __wrap_IsKeyDown(int key) {
        if (key == KEY_LEFT_CONTROL || key == KEY_RIGHT_CONTROL) {
            if (g_mockKeysDown.count(key)) return g_mockKeysDown[key];
            return false;
        }
        if (key == KEY_LEFT_SHIFT || key == KEY_RIGHT_SHIFT) return g_mockShiftDown;
        if (key == KEY_SPACE) return g_mockSpaceDown;
        return false;
    }
    
    bool __wrap_IsKeyPressed(int key) { return g_mockKeysPressed[key]; }
    bool __wrap_IsMouseButtonPressed(int button) { return g_mockMouseButtonsPressed[button]; }
    bool __wrap_IsMouseButtonDown(int button) { return g_mockMouseButtonsDown[button]; }
    bool __wrap_IsMouseButtonReleased(int button) { return g_mockMouseButtonsReleased[button]; }

    void __wrap_SetMouseCursor(int cursor) {}
    void __wrap_SetWindowTitle(const char *title) { if (title) g_lastWindowTitle = title; }

    Vector2 __wrap_GetScreenToWorld2D(Vector2 position, Camera2D camera) {
        return position; 
    }
    
    Vector2 __wrap_Vector2Add(Vector2 v1, Vector2 v2) { return {v1.x + v2.x, v1.y + v2.y}; }
    Vector2 __wrap_Vector2Scale(Vector2 v, float scale) { return {v.x * scale, v.y * scale}; }

    // --- Font / Text Wraps ---
    Vector2 __wrap_MeasureTextEx(Font font, const char *text, float fontSize, float spacing) {
        if (!text) return {0, 0};
        float width = (float)std::string(text).length() * (fontSize / 2.0f);
        return {width, fontSize};
    }

    void __wrap_DrawTextEx(Font font, const char *text, Vector2 position, float fontSize, float spacing, Color tint) {
        g_DrawTextExCount++;
        if (text) g_lastDrawTextExString = text;
        g_lastDrawTextExPos = position;
    }

    void __wrap_DrawTextPro(Font font, const char *text, Vector2 position, Vector2 origin, float rotation, float fontSize, float spacing, Color tint) {
        g_DrawTextProCount++;
        if (text) g_lastDrawTextProString = text;
        g_lastDrawTextProPos = position;
    }
    
    void __wrap_DrawText(const char *text, int posX, int posY, int fontSize, Color color) {
        __wrap_DrawTextEx({0}, text, {(float)posX, (float)posY}, (float)fontSize, 0, color);
    }

    // --- Shape Wraps ---
    void __wrap_DrawLineEx(Vector2 startPos, Vector2 endPos, float thick, Color color) {
        g_DrawLineExCount++;
        g_lastDrawLineExStart = startPos;
        g_lastDrawLineExEnd = endPos;
    }

    void __wrap_DrawRectangleRounded(Rectangle rec, float roundness, int segments, Color color) {
        g_DrawRectangleRoundedCount++;
        g_lastDrawRectangleRoundedRec = rec;
    }

    void __wrap_DrawRing(Vector2 center, float innerRadius, float outerRadius, float startAngle, float endAngle, int segments, Color color) {
        g_DrawRingCount++;
        g_lastDrawRingCenter = center;
        g_lastDrawRingRadiusOuter = outerRadius;
    }

    void __wrap_DrawRectangle(int posX, int posY, int width, int height, Color color) {
        g_DrawRectangleCount++;
        g_lastDrawRectangleRec = {(float)posX, (float)posY, (float)width, (float)height};
    }

    void __wrap_DrawRectangleRec(Rectangle rec, Color color) {
         g_DrawRectangleCount++;
         g_lastDrawRectangleRec = rec;
    }

    void __wrap_DrawRectangleLines(int posX, int posY, int width, int height, Color color) {
        g_DrawRectangleLinesCount++;
    }
    
    void __wrap_DrawRectangleLinesEx(Rectangle rec, float lineThick, Color color) {
        g_DrawRectangleLinesCount++;
    }

    void __wrap_DrawCircleV(Vector2 center, float radius, Color color) {
        g_DrawCircleCount++;
    }
    
    void __wrap_DrawCircleLinesV(Vector2 center, float radius, Color color) {
        g_DrawCircleCount++;
    }

    bool __wrap_CheckCollisionPointRec(Vector2 point, Rectangle rec) {
        return (point.x >= rec.x && point.x <= (rec.x + rec.width) &&
                point.y >= rec.y && point.y <= (rec.y + rec.height));
    }
    
    bool __wrap_CheckCollisionPointCircle(Vector2 point, Vector2 center, float radius) {
        float dx = point.x - center.x;
        float dy = point.y - center.y;
        return (dx*dx + dy*dy) <= (radius*radius);
    }

    // --- Texture / Image Wraps ---
    void __wrap_DrawTexturePro(Texture2D texture, Rectangle source, Rectangle dest, Vector2 origin, float rotation, Color tint) {
        g_DrawTextureProCount++;
    }

    Color __wrap_GetColor(unsigned int hexValue) { return {0,0,0,255}; }
    Color __wrap_Fade(Color color, float alpha) { return color; }
    
    // --- System Wraps ---
    bool __wrap_FileExists(const char *fileName) {
        if (!fileName) return false;
        return fs::exists(fileName);
    }

    bool __wrap_ExportImage(Image image, const char *fileName) { return true; }
    
    // --- Scissor / Mode Wraps ---
    void __wrap_BeginDrawing() {}
    void __wrap_EndDrawing() {}
    void __wrap_BeginMode2D(Camera2D camera) {}
    void __wrap_EndMode2D() {}
    void __wrap_ClearBackground(Color color) {}
    
    // --- Image Generation Wraps ---
    Image __wrap_GenImageColor(int width, int height, Color color) {
        Image img = {0};
        img.width = width;
        img.height = height;
        img.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
        img.mipmaps = 1;
        img.data = calloc(width * height, 4); 
        return img;
    }
    
    void __wrap_ImageFormat(Image *image, int newFormat) {
        if (!image || !image->data) return;
        image->format = newFormat;
    }

    void __wrap_ImageDrawRectangle(Image *dst, int x, int y, int width, int height, Color color) {}
    void __wrap_ImageDrawRectangleLines(Image *dst, Rectangle rec, int thick, Color color) {}
    void __wrap_ImageDraw(Image *dst, Image src, Rectangle srcRec, Rectangle dstRec, Color tint) {}
    void __wrap_ImageDrawTextEx(Image *dst, Font font, const char *text, Vector2 position, float fontSize, float spacing, Color tint) {}
    void __wrap_ImageDrawLineEx(Image *dst, Vector2 start, Vector2 end, int thick, Color color) {}
    void __wrap_ImageDrawCircle(Image *dst, int centerX, int centerY, int radius, Color color) {}
    
    void __wrap_ImageResize(Image *image, int newWidth, int newHeight) {
        if (!image) return;
        image->width = newWidth;
        image->height = newHeight;
    }
    
    void __wrap_UnloadImage(Image image) {
        if (image.data) free(image.data);
    }
    
    Image __wrap_LoadImage(const char *fileName) { 
        return __wrap_GenImageColor(100, 100, WHITE); 
    }
    Texture2D __wrap_LoadTextureFromImage(Image image) { return {1, 1, 1, 1, 7}; }
    void __wrap_UnloadTexture(Texture2D texture) {}
    
    void __wrap_DrawLine(int startPosX, int startPosY, int endPosX, int endPosY, Color color) {}
}