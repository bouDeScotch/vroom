#pragma once
#include <cmath>
#include "raylib.h"
#include "raymath.h"

struct FollowCameraConfig {
    float posStiffness   = 5.0f;   // 1/s : retard de position ~ 1/k secondes
    float angleStiffness = 2.5f;   // 1/s : retard de rotation
    float lookAheadTime  = 0.30f;  // s : on vise un point devant la voiture
    float lookAheadMax   = 20.0f;  // m
    float pitchDeg       = 15.0f;  // inclinaison : 90 = vue de dessus, plus petit = plus rasant
    float distance       = 28.0f;  // m : distance camera -> point vise (divisee par le zoom)
    float fovy           = 50.0f;  // degres
};

class FollowCamera {
public:
    explicit FollowCamera(FollowCameraConfig cfg = {}) : cfg_(cfg) {}

    void snapTo(Vector2 carPosM, float carAngleRad) { pos_ = carPosM; angle_ = carAngleRad; }

    void update(float dt, Vector2 carPosM, float carAngleRad, float forwardSpeed) {
        const float aPos = 1.0f - std::exp(-cfg_.posStiffness * dt);
        const float aAng = 1.0f - std::exp(-cfg_.angleStiffness * dt);

        const float ahead = Clamp(forwardSpeed * cfg_.lookAheadTime, -cfg_.lookAheadMax, cfg_.lookAheadMax);
        const Vector2 aim = {carPosM.x + std::cos(carAngleRad) * ahead,
                             carPosM.y + std::sin(carAngleRad) * ahead};
        pos_.x += (aim.x - pos_.x) * aPos;
        pos_.y += (aim.y - pos_.y) * aPos;

        angle_ += std::remainder(carAngleRad - angle_, 2.0f * PI) * aAng;
    }

    // Monde 3D : (x, y) du jeu devient (x, 0, y). Tout est en METRES.
    Camera3D get(float zoom) const {
        const float pitch = cfg_.pitchDeg * DEG2RAD;
        const float d = cfg_.distance / zoom;
        const float back = d * std::cos(pitch);   // recul horizontal
        const float up   = d * std::sin(pitch);   // hauteur

        Camera3D c{};
        c.target     = {pos_.x, 0.0f, pos_.y};
        c.position   = {pos_.x - std::cos(angle_) * back, up, pos_.y - std::sin(angle_) * back};
        c.up         = {0.0f, 1.0f, 0.0f};
        c.fovy       = cfg_.fovy;
        c.projection = CAMERA_PERSPECTIVE;
        return c;
    }

private:
    FollowCameraConfig cfg_;
    Vector2 pos_{};
    float angle_ = 0.0f;
};