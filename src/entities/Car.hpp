#pragma once
#include "raylib.h"
#include "raymath.h"
#include "../core/InputMapper.hpp"
#include "Engine.hpp"
#include <array>
#include <cmath>
#include <string>
#include "../physics/Lateraldynamics.hpp"

class Car {
public:
    Car() = default;
    ~Car();
    Car(const Car&) = delete;
    Car& operator=(const Car&) = delete;
    void loadSprite(const std::string& path);
    void update(float dt, const CarInput& input);
    void reset(Vector2 startPos, float startAngle);
    void draw() const;
    void drawHud() const;

    Vector2 getPosition() const { return pos; }
    float getAngle() const { return angle; }
    float getPixelsPerMeter() const { return kPixelPerMeter; }
    void setRollingCoef(float coef) { rollingCoef_ = coef; }
    void setGrip(float mu) { grip_ = mu; }

    int getCurrentGear() const { return currentGear; }
    float getSpeed() const { return sqrt(speed.x * speed.x + speed.y * speed.y); }
    float getLongitudinalSpeed() const { return forwardSpeed_; }
    float getGearRatio() const {
        const float g = gearRatios[currentGear + 1];
        if (std::fabs(g) < 1e-6f) return 0.0f;   // point mort
        return finalDrive_ / g;                  // (1/g) x pont
    }
    float getEngineRPM() const { return engine_.getRPM(); }

    std::array<Vector2, 4> getCorners() const;
    void collide(float bounce = 0.0f) {
        pos = prevPos_;
        angle = prevAngle_;
        forwardSpeed_ *= -bounce;
        lateral_.reset();
    }

    void draw3D() const;
private:
    Texture2D sprite_{};
    Vector2 pos{};
    Vector2 speed{};
    float kPixelPerMeter = 10.0f;
    float angle = 0.0f; // En radians donc la vers la droite
    float wheelDiameter = 0.653f; // Diamètre des roues en mètre (245/40 R18)
    // Inverses des rapports de la BVM6 WRX : R 3.636, 1re 3.454, 2e 1.947, 3e 1.296, 4e 0.972, 5e 0.780, 6e 0.666
    float gearRatios[8] = { -0.2750f, 0.0f, 0.2895f, 0.5136f, 0.7716f, 1.0288f, 1.2821f, 1.5015f};
    int maxGear = 6;
    float finalDrive_ = 4.111f; // Rapport de pont d'une Subaru WRX (VB, BVM6)
    int currentGear = 0; // -1 = marche arrière, 0 = point mort, 1-6 = vitesses
    Vector2 prevPos_{};
    float prevAngle_ = 0.0f;
    Engine engine_;
    Vector2 bodyCenter() const;
    Vector2 size = {4.67f, 1.83f}; // Taille de la voiture en mètre
    float forwardSpeed_ = 0.0f; // Vitesse de la voiture en m/s
    float wheelBase = 2.67f; // Empattement de la voiture en mètre
    float weight = 1530.0f; // kg
    float grip_ = 1.0f; // Adherence de la surface (0.0 = glace, 1.0 = asphalte sec)
    float bodyCenterFromRearAxle_ = 1.25f; // Distance du centre de gravité par rapport à l'essieu arrière (m)
    float steerSpeedRef_ = 18.0f; // Braquage max divisé par 2 à cette vitesse (m/s)
    LateralDynamics lateral_ = LateralDynamics(LateralParams{.mass=weight, .wheelBase=wheelBase});
    float brake = 0.0f; // 0.0f à 1.0f
    float clutchPosition = 0.0f; // Position de l'embrayage, 0 = embrayé, 1 = débrayé
    float maxSteeringAngle = 30.0f * DEG2RAD; // Angle max de braquage des roues avant en radian
    float pointMort = 0.3f; // Position du point mort sur l'embrayage
    float clutchMaxTorque_ = 500.0f; // Nm
    float rollingCoef_ = 0.015f;
    float CdA_ = 0.72f; // m^2 (Cx ~0.33 x ~2.2 m^2)
    float maxBrakeForce_ = 14000.0f; // N (~0.95 g)
    float engineTorque = 0.0f; // Sera determiné par courbe de couple, regime moteur et position de l'accelerateur plus tard, pour l'instant constant
    float wheelSpeeds[4] = {0.0f, 0.0f, 0.0f, 0.0f}; // Vitesse de chaque roue en rpm, pour l'instant on suppose que les 4 roues sont motrices et que la vitesse est la meme sur les 4 roues
    float wheelAngles[4] = {0.0f, 0.0f, 0.0f, 0.0f}; // Angle de chaque roue en radian, pour l'instant on suppose que les 2 roues avant sont directrices et que l'angle est le meme sur les 2 roues avant

    float resistanceForce(float brake) const;
    float computeLoadTorque(float dt) const;
};