
#include "Car.hpp"
#include "raymath.h"
#include <algorithm>
#include <cmath>
#include <rlgl.h>

Car::~Car() {
    if (sprite_.id != 0) UnloadTexture(sprite_);
}

void Car::loadSprite(const std::string& path) {
    if (sprite_.id != 0) UnloadTexture(sprite_);
    sprite_ = LoadTexture(path.c_str());
    if (sprite_.id == 0) return;              // echec : le rectangle bleu reste
    GenTextureMipmaps(&sprite_);              // le sprite est reduit ~10x : evite le scintillement
    SetTextureFilter(sprite_, TEXTURE_FILTER_TRILINEAR);
    SetTextureFilter(sprite_, TEXTURE_FILTER_ANISOTROPIC_16X);
}

void Car::update(float dt, const CarInput& input) {
    prevPos_ = pos_;
    prevAngle_ = angle_;

    // Gestion du changement de vitesse
    // On peut changer de vitesse seulement si l'embrayage est au moins au point mort
    // A 0 on appuie pas sur la touche donc on est embrayé, à 1 on appuie a fond donc on est debrayé
    if (input.clutch >= spec_.clutchBitePoint) {
        if (input.shiftUp) {
            if (currentGear_ < spec_.maxGear) currentGear_++;
        } else if (input.shiftDown) {
            if (currentGear_ > -1) currentGear_--;
        }
    }
    clutchPosition_ = input.clutch;

    // Gestion de la direction des roues
    const float vAbs = std::fabs(forwardSpeed_);
    const float steerRatio = vAbs / spec_.steerSpeedRef;
    const float steerLimit = spec_.maxSteeringAngle / (1.0f + steerRatio * steerRatio);
    const float delta = input.steering * steerLimit;
    wheelAngles_[0] = delta;
    wheelAngles_[1] = delta;

    // Gestion du freinage
    brake_ = input.brake;

    // Gestion du moteur
    const float r = wheelRadius();

    const float Tc = computeLoadTorque(dt); // Couple de charge en Nm
    engine_.update(dt, input.throttle, Tc, input.starter);

    const float Fdrive = Tc * getGearRatio() / r; // Force de traction en N
    const float Fres = resistanceForce(brake_);   // Force de résistance en N
    forwardSpeed_ += (Fdrive - Fres) / spec_.weight * dt; // Acceleration = Force / Masse

    // Gestion de la vitesse des roues
    const float wheelRPM = forwardSpeed_ / r * 60.0f / (2.0f * PI);
    wheelSpeeds_.fill(wheelRPM);

    // Gestion de la position et de la vitesse de la voiture
    const float brakeForce = brake_ * spec_.maxBrakeForce * std::tanh(forwardSpeed_ / 0.5f);
    const float fxFront = 0.5f * Fdrive - 0.6f * brakeForce;
    const float fxRear  = 0.5f * Fdrive - 0.4f * brakeForce;
    lateral_.step(dt, forwardSpeed_, delta, grip_, fxFront, fxRear);
    forwardSpeed_ += lateral_.longitudinalAccelExtra() * dt;

    const float vy = lateral_.lateralSpeed();
    speed_ = {forwardSpeed_ * cosf(angle_) - vy * sinf(angle_),
              forwardSpeed_ * sinf(angle_) + vy * cosf(angle_)};
    pos_.x += speed_.x * dt;
    pos_.y += speed_.y * dt;
    angle_ += lateral_.yawRate() * dt;
}

void Car::reset(Vector2 startPos, float startAngle) {
    pos_ = startPos;
    speed_ = {0.0f, 0.0f};
    forwardSpeed_ = 0.0f;
    angle_ = startAngle;
    currentGear_ = 0;
    wheelSpeeds_.fill(0.0f);
    engine_ = Engine();
    lateral_.reset();
}

void Car::collide(float bounce) {
    pos_ = prevPos_;
    angle_ = prevAngle_;
    forwardSpeed_ *= -bounce;
    lateral_.reset();
}

float Car::getSpeed() const {
    return std::sqrt(speed_.x * speed_.x + speed_.y * speed_.y);
}

float Car::getGearRatio() const {
    const float g = spec_.gearRatios[currentGear_ + 1];
    if (std::fabs(g) < 1e-6f) return 0.0f;   // point mort
    return spec_.finalDrive / g;             // (1/g) x pont
}

void Car::draw() const {
    const Vector2 bc = bodyCenter();
    const float ppm = kPixelsPerMeter;
    const Vector2 size = spec_.size;

    if (sprite_.id == 0) {                    // repli : ancien rectangle
        Rectangle r = {bc.x*ppm, bc.y*ppm, size.x*ppm, size.y*ppm};
        DrawRectanglePro(r, size/2.0f*ppm, angle_ * RAD2DEG, BLUE);
        return;
    }

    // Le sprite (nez en haut) est mis a l'echelle pour que sa HAUTEUR = longueur de la voiture
    const float scale = (size.x * ppm) / sprite_.height;
    const float w = sprite_.width * scale;
    const float h = sprite_.height * scale;

    Rectangle src = {0, 0, (float)sprite_.width, (float)sprite_.height};
    Rectangle dst = {bc.x*ppm, bc.y*ppm, w, h};
    DrawTexturePro(sprite_, src, dst, {w*0.5f, h*0.5f}, angle_ * RAD2DEG + 90.0f, WHITE);

    // Afficher la vitesse longitudinale a droite de la voiture
    // On doit prendre en compte l'angle de la voiture, qui est de toute façon toujours vertical
    // Avec la camera que l'on utilise
    const float speedKmh = forwardSpeed_ * 3.6f;
    DrawText(TextFormat("%.1f km/h", speedKmh), (bc.x + size.x*0.5f)*ppm, (bc.y - size.y*0.5f)*ppm, 20, RAYWHITE);
}

void Car::draw3D() const {
    const Vector2 bc = bodyCenter();
    const float len = spec_.size.x;                             // longueur = hauteur du sprite
    const float wid = (sprite_.id != 0) ? len * sprite_.width / sprite_.height : spec_.size.y;

    const Vector2 f = {std::cos(angle_), std::sin(angle_)};     // avant
    const Vector2 r = {-std::sin(angle_), std::cos(angle_)};    // droite

    auto vtx = [&](float u, float v) {                          // (u,v) = coordonnees dans le sprite
        const float fw = len * (0.5f - v);
        const float rt = wid * (u - 0.5f);
        rlTexCoord2f(u, v);
        rlVertex3f(bc.x + f.x * fw + r.x * rt, 0.0f, bc.y + f.y * fw + r.y * rt);
    };

    rlSetTexture(sprite_.id);                                   // 0 => rectangle uni
    rlBegin(RL_QUADS);
    if (sprite_.id != 0) rlColor4ub(255, 255, 255, 255); else rlColor4ub(0, 0, 255, 255);
    vtx(0, 0); vtx(0, 1); vtx(1, 1); vtx(1, 0);
    rlEnd();
    rlSetTexture(0);
}

void Car::drawHud() const {
    float steerAngle = atan2f(lateral_.lateralSpeed(), std::fabs(forwardSpeed_) + 0.5f) * RAD2DEG;
    const char* steerLabel = TextFormat("Steer: %.2f deg", steerAngle);
    DrawText(steerLabel, 20, GetScreenHeight() - 140, 20, RAYWHITE);
    const char* speedLabel = TextFormat("Speed: %.2f km/h", getLongitudinalSpeed() * 3.6f);
    DrawText(speedLabel, 20, GetScreenHeight() - 120, 20, RAYWHITE);
    const char* posLabel = TextFormat("Pos: (%.2f, %.2f)", pos_.x, pos_.y);
    DrawText(posLabel, 20, GetScreenHeight() - 100, 20, RAYWHITE);
    const char* angleLabel = TextFormat("Angle: %.2f rad", angle_);
    DrawText(angleLabel, 20, GetScreenHeight() - 80, 20, RAYWHITE);
    const char* gearLabel = TextFormat("Gear: %d", currentGear_);
    DrawText(gearLabel, 20, GetScreenHeight() - 60, 20, RAYWHITE);
    const char* clutchLabel = TextFormat("Clutch: %.2f", clutchPosition_);
    DrawText(clutchLabel, 20, GetScreenHeight() - 40, 20, RAYWHITE);
    const char* rpmLabel = TextFormat("RPM: %.2f", engine_.getRPM());
    DrawText(rpmLabel, 20, GetScreenHeight() - 20, 20, RAYWHITE);
    if (!engine_.isRunning()) {
        DrawText("Moteur coupe - maintenir R pour demarrer", 20, GetScreenHeight() - 150, 20, ORANGE);
    }
}

float Car::resistanceForce(float brakeLevel) const {
    // Force de résistance = force de frottement + force de traînée aérodynamique + freinage
    const float v = getLongitudinalSpeed();
    const float sgn = std::tanh(v / 0.5f); // Pour éviter les problèmes de signe à vitesse nulle
    const float rolling = rollingCoef_ * spec_.weight * 9.81f * sgn;
    const float drag = 0.5f * 1.2f * spec_.CdA * v * std::fabs(v); // 0.5 * densité de l'air * CdA * v^2
    const float braking = brakeLevel * spec_.maxBrakeForce * sgn;

    return rolling + drag + braking;
}

float Car::computeLoadTorque(float dt) const {
    if (std::fabs(getGearRatio()) < 1e-6f) return 0.0f; // Si on est au point mort, pas de couple de charge
    if (clutchPosition_ > spec_.clutchBitePoint) return 0.0f; // Si l'embrayage est débrayé, pas de couple de charge

    constexpr float rpmToRad = 2.0f * PI / 60.0f; // Conversion de RPM à rad/s
    const float wEngine = engine_.getRPM() * rpmToRad; // Vitesse angulaire du moteur en rad/s
    const float wInput = getLongitudinalSpeed() / wheelRadius() * getGearRatio(); // Vitesse angulaire d'entrée de la boîte en rad/s
    const float slip = wEngine - wInput; // Glissement entre le moteur et la boîte

    const float engagement = 1.0f - clutchPosition_;
    const float capacity = spec_.clutchMaxTorque * engagement; // Capacité de couple de l'embrayage en Nm

    const float Je = engine_.getInertia(); // Inertie du moteur en kg*m^2
    const float Jv = spec_.weight * wheelRadius() * wheelRadius() / (getGearRatio() * getGearRatio()); // Inertie équivalente de la voiture en kg*m^2
    const float Tr = resistanceForce(0.0f) * wheelRadius() / getGearRatio(); // Couple de résistance en Nm
    const float Te = engine_.getTorque(); // Couple moteur en Nm

    const float Tlock = (slip / dt + Te / Je + Tr / Jv) / (1.0f / Je + 1.0f / Jv); // Couple de verrouillage en Nm
    return std::clamp(Tlock, -capacity, capacity); // On limite le couple de charge à la capacité de l'embrayage
}

std::array<Vector2, 4> Car::getCorners() const {
    Vector2 bc = bodyCenter();
    const float hx = spec_.size.x * 0.5f, hy = spec_.size.y * 0.5f;
    const float c = cosf(angle_), s = sinf(angle_);
    std::array<Vector2, 4> out;
    const float lx[4] = {+hx, +hx, -hx, -hx};
    const float ly[4] = {+hy, -hy, -hy, +hy};
    for (int i = 0; i < 4; ++i)
        out[i] = {bc.x + lx[i] * c - ly[i] * s, bc.y + lx[i] * s + ly[i] * c};
    return out;
}

Vector2 Car::bodyCenter() const {
    const float off = spec_.bodyCenterFromRearAxle - lateral_.distCgToRear();
    return {pos_.x + off * cosf(angle_), pos_.y + off * sinf(angle_)};
}
