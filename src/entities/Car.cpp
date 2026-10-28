
#include "Car.hpp"  

void Car::update(float dt, const CarInput& input) {
    // Gestion du changement de vitesse
    // On peut changer de vitesse seulement si l'embrayage est au moins au point mort
    if (input.clutch >= pointMort) { // A 0 on appuie pas sur la touche donc on est embrayé, à 1 on appuie a fond donc on est debrayé
        if (input.shiftUp) {
            if (currentGear < 5) currentGear++;
        } else if (input.shiftDown) {
            if (currentGear > -1) currentGear--;
        }
    }
    clutchPosition = input.clutch; // On met à jour la position de l'embrayage pour l'affichage

    // Gestion de la vitesse du moteur
    // Pour l'instant on suppose que le moteur tourne a 3000rpm au max et que sa vitesse est proportionnelle a l'accelerateur
    engineRPM = input.throttle * 3000.0f;

    // Gestion de la vitesse des roues
    // Pour l'instant on suppose que les 4 roues sont motrices et à vitesse egales
    // On suppose aussi que leur vitesse est determinée par la vitesse du moteur et le rapport de boite si l'embrayage debrayé
    // Sinon, leur vitesse ralentie proportionnellement à leur vitesse actuelle (pour ralentir la voiture si on debraye)
    if (input.clutch <= pointMort && currentGear != 0) { // A 0 on appuie pas sur la touche donc on est embrayé, à 1 on appuie a fond donc on est debrayé
        float wheelSpeed = engineRPM / gearRatios[currentGear + 1]; // +1 car currentGear va de -1 à 5
        for (int i = 0; i < 4; ++i) {
            wheelSpeeds[i] = wheelSpeed;
        }
    } else {
        for (int i = 0; i < 4; ++i) {
            wheelSpeeds[i] -= wheelSpeeds[i] * dt * 0.5f; // Ralentit les roues proportionnellement à leur vitesse actuelle
        }
    }

    // Gestion de la direction des roues
    // Pour l'instant on suppose que les 2 roues avant sont directrices et que leur angle est le meme
    // Cet angle est simplement proportionnel à l'input de direction
    wheelAngles[0] = input.steering * maxSteeringAngle;
    wheelAngles[1] = input.steering * maxSteeringAngle;

    // Gestion de la position et de la vitesse de la voiture
    // Pour l'instant on suppose que la vitesse de la voiture est la meme que celle des roues
    // Et que l'angle de la voiture change proportionnellement à l'angle des roues avant
    float wheelSpeed = wheelSpeeds[0]; // On prend la vitesse de la roue avant gauche
    speed.x = wheelSpeed/60 * 2*PI*wheelDiameter * cos(angle);
    speed.y = wheelSpeed/60 * 2*PI*wheelDiameter * sin(angle);
    pos.x += speed.x * dt;
    pos.y += speed.y * dt;
    angle += wheelAngles[0] * dt * 5.0f; // On prend l'angle de la roue avant gauche
}

void Car::reset() {
    pos = {0.0f, 0.0f};
    speed = {0.0f, 0.0f};
    angle = 0.0f;
    currentGear = 0;
}

void Car::draw() const {
    Rectangle r = {pos.x, pos.y, 40.0f, 20.0f};
    DrawRectanglePro(r, {20.0f, 10.0f}, angle * RAD2DEG, RED);

    const char* posLabel = TextFormat("Pos: (%.2f, %.2f)", pos.x, pos.y);
    DrawText(posLabel, 20, GetScreenHeight() - 100, 20, RAYWHITE);
    const char* speedLabel = TextFormat("Speed: (%.2f, %.2f)", speed.x, speed.y);
    DrawText(speedLabel, 20, GetScreenHeight() - 80, 20, RAYWHITE);
    const char* angleLabel = TextFormat("Angle: %.2f rad", angle);
    DrawText(angleLabel, 20, GetScreenHeight() - 60, 20, RAYWHITE);
    const char* gearLabel = TextFormat("Gear: %d", currentGear);
    DrawText(gearLabel, 20, GetScreenHeight() - 40, 20, RAYWHITE);
    const char* clutchLabel = TextFormat("Clutch: %.2f", clutchPosition);
    DrawText(clutchLabel, 20, GetScreenHeight() - 40, 20, RAYWHITE);
    const char* rpmLabel = TextFormat("RPM: %.2f", engineRPM);
    DrawText(rpmLabel, 20, GetScreenHeight() - 20, 20, RAYWHITE);
}