#pragma once
#include "raylib.h"
#include "raymath.h"
#include "../core/InputMapper.hpp"
#include "Engine.hpp"
#include <array>
#include <cmath>

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
    float getGearRatio() const {
        const float g = gearRatios[currentGear + 1];
        if (std::fabs(g) < 1e-6f) return 0.0f;   // point mort
        return finalDrive_ / g;                  // (1/g) x pont, avec pont = 1/0.2679
    }

    std::array<Vector2, 4> getCorners() const;
    void collide(float bounce = 0.0f) {
        pos = prevPos_;
        angle = prevAngle_;
        forwardSpeed_ *= -bounce;
    }
private:
    Vector2 pos{};
    Vector2 speed{};
    float kPixelPerMeter = 10.0f;
    float angle = 0.0f; // En radians donc la vers la droite
    float wheelDiameter = 0.71f; // Diamètre des roues en mètre
    float gearRatios[9] = { -0.2794f, 0.0f, 0.3148f, 0.4474f, 0.5952f, 0.7751f, 0.9460f, 1.1389f, 1.2755f};
    int maxGear = 7;
    float finalDrive_ = 3.579f; // Rapport de pont d'une Bugatti Veyron
    int currentGear = 0; // -1 = marche arrière, 0 = point mort, 1-5 = vitesses
    Vector2 prevPos_{};
    float prevAngle_ = 0.0f;
    Engine engine_;
    Vector2 size = {2.3f, 1.5f}; // Taille de la voiture en mètre
    float forwardSpeed_ = 0.0f; // Vitesse de la voiture en m/s
    float wheelBase = 2.711f; // Empattement de la voiture en mètre
    float weight = 850.0f;
    float brake = 0.0f; // 0.0f à 1.0f
    float clutchPosition = 0.0f; // Position de l'embrayage, 0 = embrayé, 1 = débrayé
    float maxSteeringAngle = 30.0f * DEG2RAD; // Angle max de braquage des roues avant en radian
    float pointMort = 0.3f; // Position du point mort sur l'embrayage
    float clutchMaxTorque_ = 2000.0f; // Nm
    float rollingCoef_ = 0.015f;
    float CdA_ = 0.8f; // m^2
    float maxBrakeForce_ = 8000.0f; // N
    float engineTorque = 0.0f; // Sera determiné par courbe de couple, regime moteur et position de l'accelerateur plus tard, pour l'instant constant
    float wheelSpeeds[4] = {0.0f, 0.0f, 0.0f, 0.0f}; // Vitesse de chaque roue en rpm, pour l'instant on suppose que les 4 roues sont motrices et que la vitesse est la meme sur les 4 roues
    float wheelAngles[4] = {0.0f, 0.0f, 0.0f, 0.0f}; // Angle de chaque roue en radian, pour l'instant on suppose que les 2 roues avant sont directrices et que l'angle est le meme sur les 2 roues avant

    float resistanceForce(float brake) const;
    float computeLoadTorque(float dt) const;
};