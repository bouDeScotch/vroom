// src/scenes/RaceScene.hpp
#pragma once
#include <string>
#include "../core/Scene.hpp"
#include "../entities/Car.hpp"

class RaceScene : public Scene {
public:
    RaceScene(Game& game, std::string trackPath);   // 2 arguments

    void update(float dt) override;
    void draw() override;
    void handleInput() override;

private:
    std::string trackPath_;
    bool resetRequested_ = false;
    Car car_;
    InputMapper input_;
};