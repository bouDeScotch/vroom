#pragma once
#include "raylib.h"
#include "../core/InputMapper.hpp"
#include "../physics/Lateraldynamics.hpp"
#include "Engine.hpp"
#include <array>
#include <string>

// Parametres fixes de la voiture (Subaru WRX VB, BVM6). Separes de l'etat pour pouvoir
// charger une autre voiture plus tard sans toucher a la physique.
struct CarSpec {
    // Dimensions et masse
    Vector2 size = {4.67f, 1.83f};          // longueur x largeur (m)
    float wheelBase = 2.67f;                // empattement (m)
    float bodyCenterFromRearAxle = 1.25f;   // centre de la carrosserie par rapport a l'essieu arriere (m)
    float weight = 1530.0f;                 // kg
    float wheelDiameter = 0.653f;           // m (245/40 R18)

    // Transmission. Les rapports sont stockes inverses (1/g), indices : 0 = R, 1 = N, 2..7 = 1re..6e
    // Rapports reels : R 3.636, 1re 3.454, 2e 1.947, 3e 1.296, 4e 0.972, 5e 0.780, 6e 0.666
    std::array<float, 8> gearRatios = {-0.2750f, 0.0f, 0.2895f, 0.5136f, 0.7716f, 1.0288f, 1.2821f, 1.5015f};
    int maxGear = 6;
    float finalDrive = 4.111f;              // rapport de pont
    float clutchMaxTorque = 500.0f;         // Nm
    float clutchBitePoint = 0.3f;           // position d'embrayage au-dela de laquelle la boite est debrayee

    // Direction, freinage, aerodynamique
    float maxSteeringAngle = 30.0f * DEG2RAD;  // braquage max des roues avant (rad)
    float steerSpeedRef = 18.0f;               // braquage divise par 2 a cette vitesse (m/s)
    float maxBrakeForce = 14000.0f;            // N (~0.95 g)
    float CdA = 0.72f;                         // m^2 (Cx ~0.33 x ~2.2 m^2)
};

class Car {
public:
    Car() = default;
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
    LateralDynamics lateral_ = LateralDynamics(LateralParams{.mass = spec_.weight, .wheelBase = spec_.wheelBase});
    Texture2D sprite_{};

    // Cinematique
    Vector2 pos_{};
    Vector2 prevPos_{};
    Vector2 speed_{};                  // vitesse dans le repere monde (m/s)
    float angle_ = 0.0f;               // rad, 0 = vers la droite
    float prevAngle_ = 0.0f;
    float forwardSpeed_ = 0.0f;        // vitesse longitudinale (m/s)

    // Commandes appliquees (gardees pour la physique et l'affichage)
    int currentGear_ = 0;              // -1 = marche arriere, 0 = point mort, 1..6 = vitesses
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
