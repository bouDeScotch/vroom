#pragma once
#include "raylib.h"

struct Window {
    Window(int w, int h, const char* title) {
        SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
        InitWindow(w, h, title);
        SetExitKey(KEY_NULL);   // Echap sert a revenir en arriere dans les menus
        InitAudioDevice();
    }
    ~Window() { CloseAudioDevice(); CloseWindow(); }
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;
};