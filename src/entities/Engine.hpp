#pragma once
#include "raylib.h"
#include <array>

class Engine {
public:
    struct TorquePoint { float rpm; float factor; };

    struct Config {
        // Subaru WRX (FA24 2.4 L turbo, 4 cylindres a plat) : 350 Nm, 202 kW
        float idleRPM = 750.0f;
        float maxRPM = 6500.0f;
        float stallRPM = 450.0f;

        float maxTorque = 350.0f; // Nm
        float inertia = 0.2f; // kg*m^2
        float frictionTorque = 8.0f; // Nm
        float frictionPerRPM = 0.006f; // Nm/rpm
        float limiterTorque = 0.0f; // Nm

        // Demarreur : couple faible, qui s'effondre avec le regime (pas de quoi tirer la voiture en 5e)
        float starterTorque = 90.0f; // Nm a l'arret
        float starterMaxRPM = 400.0f; // Regime auquel le couple du demarreur s'annule
        float compressionTorque = 40.0f; // Nm de resistance moteur coupe (compression, frottements)
        float startRPM = 180.0f; // Regime a tenir pour que le moteur prenne
        float crankTime = 0.2f; // Duree a tenir au-dessus de startRPM (s)

        // Regulateur de ralenti (PI sur l'ecart relatif de regime)
        float idleKp = 0.5f;
        float idleKi = 0.4f;
        float idleMaxThrottle = 0.3f; // Ouverture max du filet de gaz

        // Fraction de maxTorque en fonction du regime, interpolee lineairement
        // Plateau turbo de 2000 a 5200 tr/min, puissance max 202 kW a 5600 tr/min
        std::array<TorquePoint, 8> torqueCurve{{
            {0.0f, 0.30f}, {750.0f, 0.40f}, {1500.0f, 0.65f}, {2000.0f, 1.00f},
            {5200.0f, 1.00f}, {5600.0f, 0.98f}, {6000.0f, 0.89f}, {6500.0f, 0.75f}
        }};
    };

    Engine();
    explicit Engine(const Config& config);

    void update(float dt, float throttleInput, float loadTorque, bool starter = false);
    void reset();
    void start();

    float getRPM() const { return rpm_; };
    float getTorque() const { return torque_; };
    float getThrottle() const { return throttleInput_; };
    float getInertia() const { return config_.inertia; };
    bool isRunning() const { return running_; };

private:
    Config config_;

    float rpm_ = 0.0f;
    float torque_ = 0.0f;
    float throttleInput_ = 0.0f;
    float stallTimer_ = 0.0f;
    float crankTimer_ = 0.0f;
    float idleIntegral_ = 0.0f;

    bool running_ = false;

    float getTorqueAtRPM(float rpm, float throttle) const;
};