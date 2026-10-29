
#include "Car.hpp" 
#include "Engine.hpp"
#include <algorithm>
#include <cmath>

void Car::update(float dt, const CarInput& input) {
    // Gestion du changement de vitesse
    // On peut changer de vitesse seulement si l'embrayage est au moins au point mort
    // A 0 on appuie pas sur la touche donc on est embrayé, à 1 on appuie a fond donc on est debrayé
    if (input.clutch >= pointMort) { 
        if (input.shiftUp) {
            if (currentGear < 5) currentGear++;
        } else if (input.shiftDown) {
            if (currentGear > -1) currentGear--;
        }
    }
    clutchPosition = input.clutch; // On met à jour la position de l'embrayage pour l'affichage

    // Gestion de la direction des roues
    // Pour l'instant on suppose que les 2 roues avant sont directrices et que leur angle est le meme
    // Cet angle est simplement proportionnel à l'input de direction
    wheelAngles[0] = input.steering * maxSteeringAngle;
    wheelAngles[1] = input.steering * maxSteeringAngle;

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
    // Pour l'instant on suppose que la vitesse de la voiture est la meme que celle des roues
    // Et que l'angle de la voiture change proportionnellement à l'angle des roues avant
    speed = {forwardSpeed_ * cosf(angle), forwardSpeed_ * sinf(angle)};
    pos.x += speed.x * dt;
    pos.y += speed.y * dt;
    angle += (forwardSpeed_ / wheelBase) * tanf(wheelAngles[0]) * dt;
}

void Car::reset(Vector2 startPos, float startAngle) {
    pos = startPos;
    speed = {0.0f, 0.0f};
    forwardSpeed_ = 0.0f;
    angle = startAngle;
    currentGear = 0;
    for (float& ws : wheelSpeeds) ws = 0.0f;
    engine_ = Engine();
}

void Car::draw() const {
    Rectangle r = {pos.x*kPixelPerMeter, pos.y*kPixelPerMeter, size.x*kPixelPerMeter, size.y*kPixelPerMeter};
    DrawRectanglePro(r, size/2.0f*kPixelPerMeter, angle * RAD2DEG, RED);
}

void Car::drawHud() const {
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