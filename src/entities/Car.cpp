
#include "Car.hpp" 
#include "Engine.hpp"
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
}

void Car::update(float dt, const CarInput& input) {
    prevPos_ = pos;
    prevAngle_ = angle;

    // Gestion du changement de vitesse
    // On peut changer de vitesse seulement si l'embrayage est au moins au point mort
    // A 0 on appuie pas sur la touche donc on est embrayé, à 1 on appuie a fond donc on est debrayé
    if (input.clutch >= pointMort) { 
        if (input.shiftUp) {
            if (currentGear < maxGear) currentGear++;
        } else if (input.shiftDown) {
            if (currentGear > -1) currentGear--;
        }
    }
    clutchPosition = input.clutch; // On met à jour la position de l'embrayage pour l'affichage

    // Gestion de la direction des roues
    const float vAbs = std::fabs(forwardSpeed_);
    const float steerLimit = maxSteeringAngle / (1.0f + (vAbs / steerSpeedRef_) * (vAbs / steerSpeedRef_));
    const float delta = input.steering * steerLimit;
    wheelAngles[0] = delta;
    wheelAngles[1] = delta;

    // Gestion du freinage
    brake = input.brake; // On met à jour le niveau de freinage pour l'affichage

    // Gestion du moteur
    const float r = wheelDiameter / 2.0f; // Rayon de la roue en mètre

    float Tc = computeLoadTorque(dt); // Couple de charge en Nm
    engine_.update(dt, input.throttle, Tc);

    float Fdrive = Tc * getGearRatio() / r; // Force de traction en N
    float Fres = resistanceForce(brake); // Force de résistance en N
    forwardSpeed_ += (Fdrive - Fres) / weight * dt; // Acceleration = Force / Masse

    // Gestion de la vitesse des roues
    const float wheelRPM = forwardSpeed_ / r * 60.0f / (2.0f * PI); // Vitesse de la roue en rpm
    for (float& ws : wheelSpeeds) ws = wheelRPM;

    // Gestion de la position et de la vitesse de la voiture
    const float brakeForce = brake * maxBrakeForce_ * std::tanh(forwardSpeed_ / 0.5f);
    const float fxFront = 0.5f * Fdrive - 0.6f * brakeForce;
    const float fxRear  = 0.5f * Fdrive - 0.4f * brakeForce;
    lateral_.step(dt, forwardSpeed_, delta, grip_, fxFront, fxRear);
    forwardSpeed_ += lateral_.longitudinalAccelExtra() * dt;

    const float vy = lateral_.lateralSpeed();
    speed = {forwardSpeed_ * cosf(angle) - vy * sinf(angle),
            forwardSpeed_ * sinf(angle) + vy * cosf(angle)};
    pos.x += speed.x * dt;
    pos.y += speed.y * dt;
    angle += lateral_.yawRate() * dt;
}

void Car::reset(Vector2 startPos, float startAngle) {
    pos = startPos;
    speed = {0.0f, 0.0f};
    forwardSpeed_ = 0.0f;
    angle = startAngle;
    currentGear = 0;
    for (float& ws : wheelSpeeds) ws = 0.0f;
    engine_ = Engine();
    lateral_.reset();
}

void Car::draw() const {
    const Vector2 bc = bodyCenter();
    const float ppm = kPixelPerMeter;

    if (sprite_.id == 0) {                    // repli : ancien rectangle
        Rectangle r = {bc.x*ppm, bc.y*ppm, size.x*ppm, size.y*ppm};
        DrawRectanglePro(r, size/2.0f*ppm, angle * RAD2DEG, BLUE);
        return;
    }

    // Le sprite (nez en haut) est mis a l'echelle pour que sa HAUTEUR = longueur de la voiture
    const float scale = (size.x * ppm) / sprite_.height;
    const float w = sprite_.width * scale;
    const float h = sprite_.height * scale;

    Rectangle src = {0, 0, (float)sprite_.width, (float)sprite_.height};
    Rectangle dst = {bc.x*ppm, bc.y*ppm, w, h};
    DrawTexturePro(sprite_, src, dst, {w*0.5f, h*0.5f}, angle * RAD2DEG + 90.0f, WHITE);

    // Afficher la vitesse longitudinale a droite de la voiture
    // On doit prendre en compte l'angle de la voiture, qui est de toute façon toujours vertical
    // Avec la camera que l'on utilise
    const float speedKmh = forwardSpeed_ * 3.6f;
    DrawText(TextFormat("%.1f km/h", speedKmh), (bc.x + size.x*0.5f)*ppm, (bc.y - size.y*0.5f)*ppm, 20, RAYWHITE);
}

void Car::draw3D() const {
    const Vector2 bc = bodyCenter();
    const float len = size.x;                                   // longueur = hauteur du sprite
    const float wid = (sprite_.id != 0) ? len * sprite_.width / sprite_.height : size.y;

    const Vector2 f = {std::cos(angle), std::sin(angle)};       // avant
    const Vector2 r = {-std::sin(angle), std::cos(angle)};      // droite

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
    const char* posLabel = TextFormat("Pos: (%.2f, %.2f)", pos.x, pos.y);
    DrawText(posLabel, 20, GetScreenHeight() - 100, 20, RAYWHITE);
    const char* angleLabel = TextFormat("Angle: %.2f rad", angle);
    DrawText(angleLabel, 20, GetScreenHeight() - 80, 20, RAYWHITE);
    const char* gearLabel = TextFormat("Gear: %d", currentGear);
    DrawText(gearLabel, 20, GetScreenHeight() - 60, 20, RAYWHITE);
    const char* clutchLabel = TextFormat("Clutch: %.2f", clutchPosition);
    DrawText(clutchLabel, 20, GetScreenHeight() - 40, 20, RAYWHITE);
    const char* rpmLabel = TextFormat("RPM: %.2f", engine_.getRPM());
    DrawText(rpmLabel, 20, GetScreenHeight() - 20, 20, RAYWHITE);
}

float Car::resistanceForce(float brakeLvl) const {
    // Force de résistance = force de frottement + force de traînée aérodynamique
    const float v = getLongitudinalSpeed();
    const float sgn = std::tanh(v / 0.5f); // Pour éviter les problèmes de signe à vitesse nulle
    float rolling = rollingCoef_ * weight * 9.81f * sgn; // Force de frottement = coefficient de roulement * poids * gravité
    float drag = 0.5f * 1.2f * CdA_ * v * std::fabs(v); // Force de traînée aérodynamique = 0.5 * densité de l'air * surface frontale * vitesse^2
    float braking = brakeLvl * maxBrakeForce_ * sgn; // Force de freinage = input de frein * force de freinage max

    return rolling + drag + braking;
}

float Car::computeLoadTorque(float dt) const {
    if (std::fabs(getGearRatio()) < 1e-6f) return 0.0f; // Si on est au point mort, pas de couple de charge
    if (clutchPosition > pointMort) return 0.0f; // Si l'embrayage est débrayé, pas de couple de charge

    constexpr float rpmToRad = 2.0f * PI / 60.0f; // Conversion de RPM à rad/s
    const float wEngine = engine_.getRPM() * rpmToRad; // Vitesse angulaire du moteur en rad/s
    const float wInput = getLongitudinalSpeed() / (wheelDiameter / 2.0f) * getGearRatio(); // Vitesse angulaire d'entrée de la boîte en rad/s
    const float slip = wEngine - wInput; // Glissement entre le moteur et la boîte

    float engagement = 1.0f - clutchPosition;
    const float capacity = clutchMaxTorque_ * engagement; // Capacité de couple de l'embrayage en Nm

    const float Je = engine_.getInertia(); // Inertie du moteur en kg*m^2
    const float Jv = weight * (wheelDiameter / 2.0f) * (wheelDiameter / 2.0f) / (getGearRatio() * getGearRatio()); // Inertie équivalente de la voiture en kg*m^2
    const float Tr = resistanceForce(0.0f) * (wheelDiameter / 2.0f) / getGearRatio(); // Couple de résistance en Nm
    const float Te = engine_.getTorque(); // Couple moteur en Nm

    const float Tlock = (slip / dt + Te / Je + Tr / Jv) / (1.0f / Je + 1.0f / Jv); // Couple de verrouillage en Nm
    return std::clamp(Tlock, -capacity, capacity); // On limite le couple de charge à la capacité de l'embrayage
}

std::array<Vector2, 4> Car::getCorners() const {
    Vector2 bc = bodyCenter();
    const float hx = size.x * 0.5f, hy = size.y * 0.5f;
    const float c = cosf(angle), s = sinf(angle);
    std::array<Vector2, 4> out;
    const float lx[4] = {+hx, +hx, -hx, -hx};
    const float ly[4] = {+hy, -hy, -hy, +hy};
    for (int i = 0; i < 4; ++i)
        out[i] = {bc.x + lx[i] * c - ly[i] * s, bc.y + lx[i] * s + ly[i] * c};
    return out;
}

Vector2 Car::bodyCenter() const {
    const float off = bodyCenterFromRearAxle_ - lateral_.distCgToRear();
    return {pos.x + off * cosf(angle), pos.y + off * sinf(angle)};
}