#pragma once
#include "raylib.h"

struct Window {
    Window(int w, int h, const char* title) {
        SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);
        InitWindow(w, h, title);
        InitAudioDevice();
    }
    ~Window() { CloseAudioDevice(); CloseWindow(); }
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;
};