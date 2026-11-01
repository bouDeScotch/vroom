#pragma once
#include <vector>
#include "../core/Scene.hpp"

// Premier ecran : choix de la voiture, puis menu des circuits.
class CarSelectScene : public Scene {
public:
    explicit CarSelectScene(Game& game, int initialIndex = 0);

    void handleInput() override;
    void update(float) override {}
    void draw() override;

private:
    struct Stats {
        float powerKw = 0.0f;
        float peakTorque = 0.0f;  // Nm
        float weight = 0.0f;      // kg
        float Vmax = 0.0f;        // km/h
    };

    std::vector<Stats> stats_;  // une entree par voiture du catalogue
    int selected_ = 0;
};
