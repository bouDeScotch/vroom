// src/core/Game.cpp
#include "Game.hpp"
#include "../scenes/MenuScene.hpp"
#include "Paths.hpp"
#include <algorithm>

Game::Game()
    : window_(1280, 720, "vroom"),
      assets_(assetPath()) {
    changeScene<MenuScene>();
    scene_ = std::move(pending_);
}

void Game::run() {
    float acc = 0.0f;
    while (running_ && !WindowShouldClose()) {
        if (pending_) scene_ = std::move(pending_);
        
        scene_->handleInput();

        acc += std::min(GetFrameTime(), 0.25f);   // évite la spirale de la mort
        while (acc >= kStep) {
            scene_->update(kStep);
            acc -= kStep;
        }

        BeginDrawing();
            ClearBackground(SKYBLUE);
            scene_->draw();
            DrawFPS(10, 10);
        EndDrawing();
    }
}