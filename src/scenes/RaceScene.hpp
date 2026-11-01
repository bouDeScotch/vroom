#pragma once
#include <string>
#include "raylib.h"
#include "../core/Scene.hpp"
#include "../entities/Car.hpp"
#include "../entities/CarModel.hpp"
#include "../entities/Engine.hpp"
#include "../world/Track.hpp"
#include "../audio/Enginesound.hpp"
#include "../core/FollowCamera.hpp"
#include "../world/LapTime.hpp"

class RaceScene : public Scene {
public:
    RaceScene(Game& game, const CarModel& carModel, std::string trackPath, std::string checkpointsPath = "");

    void update(float dt) override;
    void draw() override;
    void handleInput() override;

private:
    std::string trackPath_;
    float lineCrossed_ = -1.0f;  // pour detecter le passage de la ligne de depart
    bool resetRequested_ = false;
    Track track_;            // declare apres trackPath_ : construit a partir de lui
    Car car_;
    LapTimer lapTimer_;
    bool raceStarted_ = false;
    int nextCheckpoint_ = 1;  // prochain checkpoint a franchir; > checkpointCount() = tour valide
    InputMapper input_;
    Surface surface_ = Surface::Asphalt;  // surface sous la voiture (mise a jour dans update)
    FollowCamera camera_;
    EngineSound engineSound_;
    float zoom_ = 1.0f;      // molette de la souris pour regler
};