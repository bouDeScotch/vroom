#pragma once
#include "raylib.h"
#include "raymath.h"
#include "../core/InputMapper.hpp"
#include "Engine.hpp"

class Car {
public:
    Car() = default;
    void update(float dt, const CarInput& input);
    void reset(Vector2 startPos, float startAngle);
    void draw() const;
    void drawHud() const;

    Vector2 getPosition() const { return pos; }
    float getAngle() const { return angle; }
    float getPixelsPerMeter() const { return kPixelPerMeter; }
    void setRollingCoef(float coef) { rollingCoef_ = coef; }

    int getCurrentGear() const { return currentGear; }
    float getSpeed() const { return sqrt(speed.x * speed.x + speed.y * speed.y); }
    float getLongitudinalSpeed() const { return forwardSpeed_; }
    float getGearRatio() const { return gearRatios[currentGear + 1]; }
private:
    Vector2 pos{};
    Vector2 speed{};
    float kPixelPerMeter = 10.0f;
    float angle = 0.0f; // En radians donc la vers la droite
    float wheelDiameter = 0.62f; // Diamètre des roues en mètre
    float gearRatios[7] = { -14.0f, 0.0f, 14.0f, 8.4f, 5.8f, 4.4f, 3.5f };
    int currentGear = 0; // -1 = marche arrière, 0 = point mort, 1-5 = vitesses

private:
    Engine engine_;
    Vector2 size = {2.3f, 1.5f}; // Taille de la voiture en mètre
    float forwardSpeed_ = 0.0f; // Vitesse de la voiture en m/s
    float wheelBase = 2.5f; // Empattement de la voiture en mètre
    float weight = 1500.0f;
    float brake = 0.0f; // 0.0f à 1.0f
    float clutchPosition = 0.0f; // Position de l'embrayage, 0 = embrayé, 1 = débrayé
    float maxSteeringAngle = 30.0f * DEG2RAD; // Angle max de braquage des roues avant en radian
    float pointMort = 0.3f; // Position du point mort sur l'embrayage
    float clutchMaxTorque_ = 300.0f; // Nm
    float rollingCoef_ = 0.015f;
    float CdA_ = 0.7f; // m^2
    float maxBrakeForce_ = 8000.0f; // N
    float engineTorque = 0.0f; // Sera determiné par courbe de couple, regime moteur et position de l'accelerateur plus tard, pour l'instant constant
    float wheelSpeeds[4] = {0.0f, 0.0f, 0.0f, 0.0f}; // Vitesse de chaque roue en rpm, pour l'instant on suppose que les 4 roues sont motrices et que la vitesse est la meme sur les 4 roues
    float wheelAngles[4] = {0.0f, 0.0f, 0.0f, 0.0f}; // Angle de chaque roue en radian, pour l'instant on suppose que les 2 roues avant sont directrices et que l'angle est le meme sur les 2 roues avant

    float resistanceForce(float brake) const;
    float computeLoadTorque(float dt) const;
};