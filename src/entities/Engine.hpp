#pragma once
#include "raylib.h"

class Engine {
public:
    struct Config {
        float idleRPM = 800.0f;
        float maxRPM = 6800.0f;
        float stallRPM = 450.0f;

        float maxTorque = 1600.0f; // Nm
        float inertia = 0.3f; // kg*m^2
        float frictionTorque = 10.0f; // Nm
        float frictionPerRPM = 0.01f; // Nm/rpm
        float limiterTorque = 0.0f; // Nm
    };

    Engine();
    explicit Engine(const Config& config);

    void update(float dt, float throttleInput, float loadTorque);
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

    bool running_ = false;

    float getTorqueAtRPM(float rpm, float throttle) const;
};