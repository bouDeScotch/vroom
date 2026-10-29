#include "Engine.hpp"

#include <algorithm>
#include <cmath>

Engine::Engine() : Engine(Config{}) {}

Engine::Engine(const Config& config) : config_(config) {
    reset();
}

float Engine::getTorqueAtRPM(float rpm, float throttle) const {
    const auto& c = config_.torqueCurve;

    // Interpolation lineaire entre les points de la courbe, valeurs extremes au-dela
    float factor = c.back().factor;
    if (rpm <= c.front().rpm) {
        factor = c.front().factor;
    } else {
        for (size_t i = 1; i < c.size(); ++i) {
            if (rpm <= c[i].rpm) {
                const float t = (rpm - c[i - 1].rpm) / (c[i].rpm - c[i - 1].rpm);
                factor = c[i - 1].factor + t * (c[i].factor - c[i - 1].factor);
                break;
            }
        }
    }
    return config_.maxTorque * factor * throttle;
}

void Engine::update(float dt, float throttleInput, float loadTorque) {
    if (dt <= 0.0f) return;

    // Il faut une manière de demarrer le moteur
    // On va implementer un starter
    if (!running_ && throttleInput_ > 0.0f) {
        start();
    }

    throttleInput_ = std::clamp(throttleInput, 0.0f, 1.0f);

    if (!running_) {
        // Si le moteur est arrêté, on ne fait rien
        rpm_ = 0.0f;
        torque_ = 0.0f;
        return;
    }

    const float omega = rpm_ * (2.0f * PI / 60.0f); // Convertir RPM en rad/s
    const float limiterStart = config_.maxRPM - 150.0f;
    const float limiterFactor = std::clamp(
        (config_.maxRPM - rpm_) / (config_.maxRPM - limiterStart),
        0.0f, 1.0f
    );

    // Calculer le couple moteur en fonction du régime et de l'entrée d'accélérateur
    torque_ = getTorqueAtRPM(rpm_, throttleInput_) * limiterFactor;

    // Calculer l'accélération angulaire en fonction du couple moteur, du couple de charge et du couple de friction
    const float friction = config_.frictionTorque + config_.frictionPerRPM * rpm_;
    float angularAcceleration = (torque_ - loadTorque - friction) / config_.inertia;
    rpm_ += angularAcceleration * dt * (60.0f / (2.0f * PI));

    // Vérifier si le moteur est en dessous du régime de calage
    if (rpm_ < config_.stallRPM) {
        stallTimer_ += dt;
        if (stallTimer_ > 1.0f) {
            // Si le moteur est en dessous du régime de calage pendant plus d'une seconde, il cale
            running_ = false;
            rpm_ = 0.0f;
            torque_ = 0.0f;
        }
    } else {
        stallTimer_ = 0.0f; // Réinitialiser le timer de calage si le régime est au-dessus du seuil
    }
}

void Engine::reset() {
    rpm_ = 0.0f;
    torque_ = 0.0f;
    throttleInput_ = 0.0f;
    stallTimer_ = 0.0f;
    running_ = false;
}

void Engine::start() {
    if (!running_) {
        running_ = true;
        rpm_ = config_.idleRPM; // Démarrer le moteur au régime de ralenti
        stallTimer_ = 0.0f; // Réinitialiser le timer de calage
    }
}