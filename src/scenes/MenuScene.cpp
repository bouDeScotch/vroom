// src/scenes/MenuScene.cpp
#include "scenes/MenuScene.hpp"
#include "../core/Game.hpp"
#include "../core/Paths.hpp"
#include "RaceScene.hpp"
#include "raylib.h"

MenuScene::MenuScene(Game& game) : Scene(game) {
    items_ = {
        {"Jouer",   [this] { game_.changeScene<RaceScene>(assetPath("Tracks/Track2.png")); }},
        {"Circuit 1 (checkpoints)", [this] { game_.changeScene<RaceScene>(assetPath("Tracks/Track1.png"), assetPath("Tracks/Checkpoints1.png")); }},
        {"Quitter", [this] { game_.quit(); }},
    };
}

void MenuScene::handleInput() {
    if (IsKeyPressed(KEY_DOWN)) selected_ = (selected_ + 1) % items_.size();
    if (IsKeyPressed(KEY_UP))   selected_ = (selected_ + items_.size() - 1) % items_.size();

    if (IsKeyPressed(KEY_ENTER)) items_[selected_].action();
}

void MenuScene::draw() {
    const int w = GetScreenWidth();
    const int h = GetScreenHeight();
    
    const char* title = "MON JEU";
    DrawText(title, (w - MeasureText(title, 60)) / 2, h / 5, 60, RAYWHITE);

    const int fontSize = 30;
    const int lineH = 50;
    const int startY = h / 2;

    for (int i = 0; i < static_cast<int>(items_.size()); ++i) {
        const char* label = items_[i].label.c_str();
        const bool sel = (i == selected_);
        const Color c = sel ? YELLOW : GRAY;
        const int x = (w - MeasureText(label, fontSize)) / 2;

        DrawText(label, x, startY + i * lineH, fontSize, c);
        if (sel) DrawText(">", x - 30, startY + i * lineH, fontSize, c);
    }
}