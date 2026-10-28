#pragma once
#include "raylib.h"
#include "raymath.h"
#include "../core/InputMapper.hpp"

class Car {
public:
    Vector2 pos{};
    Vector2 speed{};
    float angle = 0.0f; // En radians donc la vers la droite
    float wheelDiameter = 0.5f; // Diamètre des roues en mètre
    float gearRatios[7] = { // Boite 5 vitesses + marche arrière + point mort
                            // Vitesse de la roue = vitesse du moteur / rapport de boite
        -31.4f,
        0.0f,
        31.4f,
        13.5f,
        7.9f,
        5.9f,
        4.28f
    };
    int currentGear = 0; // -1 = marche arrière, 0 = point mort, 1-5 = vitesses

    void update(float dt, const CarInput& input);
    void reset();
    void draw() const;

private:
    float weight = 1500.0f;
    float clutchPosition = 0.0f; // Position de l'embrayage, 0 = embrayé, 1 = débrayé
    float maxSteeringAngle = 30.0f * DEG2RAD; // Angle max de braquage des roues avant en radian
    float pointMort = 0.3f; // Position du point mort sur l'embrayage
    float engineRPM = 0.0f;
    float engineTorque = 0.0f; // Sera determiné par courbe de couple, regime moteur et position de l'accelerateur plus tard, pour l'instant constant
    float wheelSpeeds[4] = {0.0f, 0.0f, 0.0f, 0.0f}; // Vitesse de chaque roue en rpm, pour l'instant on suppose que les 4 roues sont motrices et que la vitesse est la meme sur les 4 roues
    float wheelAngles[4] = {0.0f, 0.0f, 0.0f, 0.0f}; // Angle de chaque roue en radian, pour l'instant on suppose que les 2 roues avant sont directrices et que l'angle est le meme sur les 2 roues avant
};