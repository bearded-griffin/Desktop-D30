#pragma once

#ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif

    // Rename conflicting Windows functions before including windows.h
    #define Rectangle _win_Rectangle
    #define CloseWindow _win_CloseWindow
    #define ShowCursor _win_ShowCursor
    #define LoadImage _win_LoadImage
    #define DrawText _win_DrawText
    #define DrawTextEx _win_DrawTextEx

    #include <windows.h>
    #include <shellapi.h>

    // Undefine them so they don't affect raylib or our code
    #undef Rectangle
    #undef CloseWindow
    #undef ShowCursor
    #undef LoadImage
    #undef DrawText
    #undef DrawTextEx
#endif
