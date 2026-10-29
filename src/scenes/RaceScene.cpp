#include "scenes/RaceScene.hpp"
#include "scenes/MenuScene.hpp"
#include "core/Game.hpp"
#include "raylib.h"
#include "raymath.h"
#include <utility>

namespace {
const char* surfaceName(Surface s) {
    switch (s) {
        case Surface::Asphalt: return "Asphalte";
        case Surface::Gravel:  return "Gravier";
        case Surface::Grass:   return "Herbe";
        case Surface::Wall:    return "Mur";
    }
    return "?";
}
}  // namespace

RaceScene::RaceScene(Game& game, std::string trackPath)
    : Scene(game),
      trackPath_(std::move(trackPath)),
      track_(trackPath_, TrackConfig{0.5f, 0.0f}) {  // 0.5 m/pixel, depart vers la droite (0 deg)
    car_.reset(track_.startPosition(), track_.startAngleRad());
}

void RaceScene::handleInput() {
    input_.poll();
    if (IsKeyPressed(KEY_BACKSPACE)) resetRequested_ = true;
    if (IsKeyPressed(KEY_ESCAPE)) game_.quit();

    // Zoom a la molette (min 0.2 pour eviter le scintillement de la texture reduite)
    zoom_ = Clamp(zoom_ * (1.0f + GetMouseWheelMove() * 0.1f), 0.2f, 3.0f);
}

void RaceScene::update(float dt) {
    if (resetRequested_) {
        car_.reset(track_.startPosition(), track_.startAngleRad());
        resetRequested_ = false;
    }

    // La surface sous la voiture fixe la resistance au roulement
    surface_ = track_.surfaceAt(car_.getPosition());
    car_.setRollingCoef(Track::rollingCoeff(surface_));

    const CarInput& input = input_.step(dt);
    car_.update(dt, input);

    for (const Vector2& corner : car_.getCorners()) {
        if (track_.isWall(corner)) { car_.collide(); break; }
    }
}

void RaceScene::draw() {
    const float ppm = car_.getPixelsPerMeter();
    const Vector2 p = car_.getPosition();

    // Camera qui suit la voiture (orientation fixe)
    camera_.offset = {GetScreenWidth() * 0.5f, GetScreenHeight() * 0.5f};
    camera_.target = {p.x * ppm, p.y * ppm};
    camera_.rotation = 0.0f;
    camera_.zoom = zoom_;

    BeginMode2D(camera_);
    track_.draw(ppm);
    car_.draw();   // uniquement la carrosserie, en coordonnees monde
    EndMode2D();

    // HUD en coordonnees ecran, apres EndMode2D
    DrawText("Backspace: retour au depart | Molette: zoom", 20, 20, 20, RAYWHITE);
    DrawText(TextFormat("Surface: %s | Zoom: %.2f", surfaceName(surface_), zoom_), 20, 45, 20, RAYWHITE);
    int g = car_.getCurrentGear();
    const char* label = (g == -1) ? "R" : (g == 0) ? "N" : TextFormat("%d", g);
    DrawText(label, GetScreenWidth() - 60, GetScreenHeight() - 40, 40, RAYWHITE);
    car_.drawHud();
}