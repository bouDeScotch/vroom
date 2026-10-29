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

void Engine::update(float dt, float throttleInput, float loadTorque, bool starter) {
    if (dt <= 0.0f) return;

    throttleInput_ = std::clamp(throttleInput, 0.0f, 1.0f);

    if (!running_) {
        // Moteur coupe : seul le demarreur fournit du couple, et il subit la charge de la transmission
        torque_ = starter
            ? config_.starterTorque * std::clamp(1.0f - rpm_ / config_.starterMaxRPM, 0.0f, 1.0f)
            : 0.0f;
        const float drag = rpm_ > 0.0f ? config_.compressionTorque : 0.0f;
        rpm_ += (torque_ - loadTorque - drag) / config_.inertia * dt * (60.0f / (2.0f * PI));
        rpm_ = std::max(rpm_, 0.0f);

        // Le moteur prend s'il tient assez de regime assez longtemps
        if (starter && rpm_ >= config_.startRPM) {
            crankTimer_ += dt;
            if (crankTimer_ >= config_.crankTime) start();
        } else {
            crankTimer_ = 0.0f;
        }
        return;
    }

    const float omega = rpm_ * (2.0f * PI / 60.0f); // Convertir RPM en rad/s
    const float limiterStart = config_.maxRPM - 150.0f;
    const float limiterFactor = std::clamp(
        (config_.maxRPM - rpm_) / (config_.maxRPM - limiterStart),
        0.0f, 1.0f
    );

    // Regulation du ralenti : filet de gaz (PI) pour tenir idleRPM, le conducteur peut ouvrir davantage
    const float idleError = (config_.idleRPM - rpm_) / config_.idleRPM;
    idleIntegral_ = std::clamp(idleIntegral_ + config_.idleKi * idleError * dt, 0.0f, config_.idleMaxThrottle);
    const float idleThrottle = std::clamp(idleIntegral_ + config_.idleKp * idleError, 0.0f, config_.idleMaxThrottle);
    const float throttle = std::max(throttleInput_, idleThrottle);

    // Calculer le couple moteur en fonction du régime et de l'entrée d'accélérateur
    torque_ = getTorqueAtRPM(rpm_, throttle) * limiterFactor;

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
    crankTimer_ = 0.0f;
    idleIntegral_ = 0.0f;
    running_ = false;
}

void Engine::start() {
    if (!running_) {
        running_ = true;
        rpm_ = config_.idleRPM; // Démarrer le moteur au régime de ralenti
        stallTimer_ = 0.0f; // Réinitialiser le timer de calage
        crankTimer_ = 0.0f;
        idleIntegral_ = 0.1f; // Ouverture approximative du ralenti, affinee par le regulateur
    }
}