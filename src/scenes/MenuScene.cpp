// src/scenes/MenuScene.cpp
#include "scenes/MenuScene.hpp"
#include "../core/Game.hpp"
#include "../core/Paths.hpp"
#include "../entities/CarCatalog.hpp"
#include "CarSelectScene.hpp"
#include "RaceScene.hpp"
#include "raylib.h"

MenuScene::MenuScene(Game& game, int carIndex) : Scene(game), carIndex_(carIndex) {
    items_ = {
        {"Circuit 1",   [this] { game_.changeScene<RaceScene>(carCatalog()[carIndex_], assetPath("Tracks/Track1.trk"), assetPath("Tracks/Checkpoints1.cpt")); }},
        {"Circuit 2", [this] { game_.changeScene<RaceScene>(carCatalog()[carIndex_], assetPath("Tracks/Track2.trk"), assetPath("Tracks/Checkpoints2.cpt")); }},
        {"Straight (Testing)", [this] { game_.changeScene<RaceScene>(carCatalog()[carIndex_], assetPath("Tracks/Straight.trk")); }},
        {"Quitter", [this] { game_.quit(); }},
    };
}

void MenuScene::handleInput() {
    if (IsKeyPressed(KEY_DOWN)) selected_ = (selected_ + 1) % items_.size();
    if (IsKeyPressed(KEY_UP))   selected_ = (selected_ + items_.size() - 1) % items_.size();

    if (IsKeyPressed(KEY_ENTER)) items_[selected_].action();
    if (IsKeyPressed(KEY_ESCAPE)) game_.changeScene<CarSelectScene>(carIndex_);   // retour au choix de voiture
}

void MenuScene::draw() {
    const int w = GetScreenWidth();
    const int h = GetScreenHeight();
    
    const char* title = "MON JEU";
    DrawText(title, (w - MeasureText(title, 60)) / 2, h / 5, 60, RAYWHITE);

    const char* carName = carCatalog()[carIndex_].name.c_str();
    DrawText(carName, (w - MeasureText(carName, 28)) / 2, h / 5 + 80, 28, YELLOW);

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