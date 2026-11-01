#pragma once
// Modele bicyclette dynamique (vitesse laterale vy + vitesse de lacet r), reference au centre de gravite.
// Les pneus suivent une courbe de Pacejka simplifiee : l'adherence sature, donc la voiture
// peut sous-virer ou deraper au lieu de suivre les roues comme sur des rails.
// Sous ~4 m/s, on bascule en douceur vers le modele cinematique (rotation autour de l'essieu arriere).
// Aucune dependance a raylib.

#include <algorithm>
#include <cmath>

struct LateralParams {
    float mass = 1530.0f;
    float wheelBase = 2.67f;
    float frontWeightFrac = 0.58f;        // part du poids sur l'essieu avant
    float yawInertia = 2500.0f;           // kg*m^2
    float cornerStiffFront = 120000.0f;   // N/rad, essieu avant (mu = 1)
    float cornerStiffRear = 120000.0f;    // N/rad, essieu arriere
    float gripFront = 0.94f;              // coef. d'adherence relatif avant/arriere : <1 a l'avant = sous-virage a la limite
    float gripRear = 1.00f;
    float shapeC = 1.6f;                  // forme de la courbe Pacejka (pic a ~10 deg, perte de grip au-dela)
    float vBlendLow = 1.5f;               // en dessous : cinematique pur
    float vBlendHigh = 4.0f;              // au dessus : dynamique pur
    float vMinSlip = 2.0f;                // plancher de vitesse pour le calcul des angles de derive
};

class LateralDynamics {
public:
    LateralDynamics() = default;
    explicit LateralDynamics(const LateralParams& p) : p_(p) {}

    float distCgToFront() const { return p_.wheelBase * (1.0f - p_.frontWeightFrac); }  // a
    float distCgToRear()  const { return p_.wheelBase * p_.frontWeightFrac; }           // b

    void reset() { vy_ = r_ = axExtra_ = slipF_ = slipR_ = fyF_ = fyR_ = 0.0f; }

    // Adherence maximale de chaque essieu (N) a la surface mu : force combinee (longitudinale + laterale) qu'il peut transmettre.
    float frontFrictionLimit(float mu) const { return mu * p_.gripFront * p_.mass * 9.81f * p_.frontWeightFrac; }
    float rearFrictionLimit(float mu) const { return mu * p_.gripRear * p_.mass * 9.81f * (1.0f - p_.frontWeightFrac); }
    float lateralForceFront() const { return fyF_; }  // N, dernier pas de temps
    float lateralForceRear() const { return fyR_; }

    float lateralSpeed() const { return vy_; }        // m/s, repere voiture : positif vers la droite de la voiture (y vers le bas a l ecran)
    float yawRate() const { return r_; }              // rad/s
    float longitudinalAccelExtra() const { return axExtra_; }  // a ajouter a l'acceleration longitudinale (m/s^2)
    float slipFront() const { return slipF_; }        // rad, pour le debug / la fumee des pneus
    float slipRear() const { return slipR_; }

    // vx : vitesse longitudinale (m/s). delta : braquage (rad). mu : adherence de la surface.
    // fxFront / fxRear : forces longitudinales par essieu (N), elles consomment une part de l'adherence.
    void step(float dt, float vx, float delta, float mu, float fxFront, float fxRear) {
        const float L = p_.wheelBase;
        const float a = distCgToFront();
        const float b = distCgToRear();
        const float g = 9.81f;
        const float nF = p_.mass * g * b / L;
        const float nR = p_.mass * g * a / L;
        const float cd = std::cos(delta), sd = std::sin(delta);

        // Vitesse des points de contact, dans le repere de chaque essieu
        const float vyF = vy_ + a * r_;
        const float vyR = vy_ - b * r_;
        const float vLatF = vyF * cd - vx * sd;
        const float vLongF = vx * cd + vyF * sd;
        slipF_ = std::atan2(vLatF, std::max(std::fabs(vLongF), p_.vMinSlip));
        slipR_ = std::atan2(vyR, std::max(std::fabs(vx), p_.vMinSlip));

        // Force laterale dans le repere de la roue : elle s'oppose au glissement
        const float fyF = -tireForce(slipF_, nF, mu * p_.gripFront, fxFront, p_.cornerStiffFront);
        const float fyR = -tireForce(slipR_, nR, mu * p_.gripRear, fxRear, p_.cornerStiffRear);
        fyF_ = fyF;
        fyR_ = fyR;

        const float fyBody = fyF * cd + fyR;
        axExtra_ = vy_ * r_ - fyF * sd / p_.mass;

        vy_ += (fyBody / p_.mass - vx * r_) * dt;
        r_ += (a * fyF * cd - b * fyR) / p_.yawInertia * dt;

        // Basse vitesse : melange avec le modele cinematique (pas de derive, rotation autour de l'essieu arriere)
        const float rKin = vx / L * std::tan(delta);
        const float vyKin = b * rKin;
        const float w = smoothstep(p_.vBlendLow, p_.vBlendHigh, std::hypot(vx, vy_));
        vy_ = w * vy_ + (1.0f - w) * vyKin;
        r_ = w * r_ + (1.0f - w) * rKin;
    }

private:
    static float smoothstep(float e0, float e1, float x) {
        const float t = std::clamp((x - e0) / (e1 - e0), 0.0f, 1.0f);
        return t * t * (3.0f - 2.0f * t);
    }

    float tireForce(float slip, float n, float mu, float fx, float cAlpha) const {
        const float C = p_.shapeC;
        const float B = cAlpha / (C * n);                 // pente a l'origine = mu * cAlpha
        const float dFull = mu * n;
        // Cercle de friction : la force longitudinale reduit l'adherence laterale (min 15 %)
        const float dy = std::sqrt(std::max(dFull * dFull - fx * fx, 0.0225f * dFull * dFull));
        return dy * std::sin(C * std::atan(B * slip));
    }

    LateralParams p_{};
    float vy_ = 0.0f, r_ = 0.0f, axExtra_ = 0.0f, slipF_ = 0.0f, slipR_ = 0.0f, fyF_ = 0.0f, fyR_ = 0.0f;
};