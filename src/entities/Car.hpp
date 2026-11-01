#pragma once
#include "raylib.h"
#include "../core/InputMapper.hpp"
#include "../physics/Lateraldynamics.hpp"
#include "CarModel.hpp"
#include "Engine.hpp"
#include <array>
#include <string>

class Car {
public:
    explicit Car(const CarModel& model);
    ~Car();
    Car(const Car&) = delete;
    Car& operator=(const Car&) = delete;

    void loadSprite(const std::string& path);
    void reset(Vector2 startPos, float startAngle);
    void update(float dt, const CarInput& input);
    void collide(float bounce = 0.0f);

    void draw() const;
    void draw3D() const;
    void drawHud() const;

    // Etat
    Vector2 getPosition() const { return pos_; }
    float getAngle() const { return angle_; }
    float getSpeed() const;
    float getLongitudinalSpeed() const { return forwardSpeed_; }
    int getCurrentGear() const { return currentGear_; }
    float getGearRatio() const;
    float getEngineRPM() const { return engine_.getRPM(); }
    int getMaxGear() const { return spec_.maxGear; }
    Vector2 getPrevPos() const { return prevPos_; }
    std::array<Vector2, 4> getCorners() const;

    // Rendu
    float getPixelsPerMeter() const { return kPixelsPerMeter; }

    // Reglages venant de la piste
    void setRollingCoef(float coef) { rollingCoef_ = coef; }
    void setGrip(float mu) { grip_ = mu; }

private:
    static constexpr float kPixelsPerMeter = 10.0f;

    Vector2 bodyCenter() const;
    float wheelRadius() const { return spec_.wheelDiameter * 0.5f; }
    float resistanceForce(float brakeLevel) const;
    float computeLoadTorque(float dt) const;

    CarSpec spec_;
    Engine engine_;
    LateralDynamics lateral_;
    Texture2D sprite_{};

    // Cinematique
    Vector2 pos_{};
    Vector2 prevPos_{};
    Vector2 speed_{};                  // vitesse dans le repere monde (m/s)
    float angle_ = 0.0f;               // rad, 0 = vers la droite
    float prevAngle_ = 0.0f;
    float forwardSpeed_ = 0.0f;        // vitesse longitudinale (m/s)

    // Commandes appliquees (gardees pour la physique et l'affichage)
    int currentGear_ = 0;              // -1 = marche arriere, 0 = point mort, 1..maxGear = vitesses
    float brake_ = 0.0f;               // 0..1
    float clutchPosition_ = 0.0f;      // 0 = embraye, 1 = debraye

    // Conditions de piste
    float grip_ = 1.0f;                // 0 = glace, 1 = asphalte sec
    float rollingCoef_ = 0.015f;

    // Roues : pour l'instant les 4 roues sont motrices et tournent a la meme vitesse,
    // seules les 2 roues avant (indices 0 et 1) braquent.
    std::array<float, 4> wheelSpeeds_{};   // tr/min
    std::array<float, 4> wheelAngles_{};   // rad
};
