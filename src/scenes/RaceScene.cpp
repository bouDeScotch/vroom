#include "scenes/RaceScene.hpp"
#include "scenes/MenuScene.hpp"
#include "core/Game.hpp"
#include "raylib.h"

void RaceScene::handleInput() {
    input_.poll();
    if (IsKeyPressed(KEY_BACKSPACE)) resetRequested_ = true;
    if (IsKeyPressed(KEY_ESCAPE)) game_.quit();
}

RaceScene::RaceScene(Game& game, std::string trackPath)
    : Scene(game), trackPath_(std::move(trackPath)) {}

void RaceScene::update(float dt) {
    const CarInput& input = input_.step(dt);
    car_.update(dt, input);
    
    if (resetRequested_) {
        car_.reset();
        resetRequested_ = false;
    }
}

void RaceScene::draw() {
    DrawText("Course (Backspace pour revenir)", 20, 20, 20, RAYWHITE);
    int g = car_.currentGear;
    const char* label = (g == -1) ? "R" : (g == 0) ? "N" : TextFormat("%d", g);
    DrawText(label, 20, GetScreenHeight() - 60, 40, RAYWHITE);
    car_.draw();
}