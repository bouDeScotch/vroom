#pragma once
#include <string>
#include "raylib.h"
#include "../core/Scene.hpp"
#include "../entities/Car.hpp"
#include "../entities/Engine.hpp"
#include "../world/Track.hpp"

class RaceScene : public Scene {
public:
    RaceScene(Game& game, std::string trackPath);   // 2 arguments

    void update(float dt) override;
    void draw() override;
    void handleInput() override;

private:
    std::string trackPath_;
    bool resetRequested_ = false;
    Track track_;            // declare apres trackPath_ : construit a partir de lui
    Car car_;
    InputMapper input_;
    Surface surface_ = Surface::Asphalt;  // surface sous la voiture (mise a jour dans update)
    Camera2D camera_{};
    float zoom_ = 0.6f;      // molette de la souris pour regler
};