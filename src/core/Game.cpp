// src/core/Game.cpp
#include "Game.hpp"
#include "../scenes/MenuScene.hpp"
#include <algorithm>

Game::Game()
    : window_(1280, 720, "vroom"),
      assets_(ASSETS_PATH) {
    changeScene<MenuScene>();
    scene_ = std::move(pending_);
}

void Game::run() {
    float acc = 0.0f;
    while (running_ && !WindowShouldClose()) {
        if (pending_) scene_ = std::move(pending_);

        if (IsKeyPressed(KEY_DOWN)) TraceLog(LOG_INFO, "DOWN detecte");
        scene_->handleInput();

        acc += std::min(GetFrameTime(), 0.25f);   // évite la spirale de la mort
        while (acc >= kStep) {
            scene_->update(kStep);
            acc -= kStep;
        }

        BeginDrawing();
            ClearBackground(BLACK);
            scene_->draw();
            DrawFPS(10, 10);
        EndDrawing();
    }
}