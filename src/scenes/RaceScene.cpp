#include "scenes/RaceScene.hpp"
#include "scenes/MenuScene.hpp"
#include "core/Game.hpp"
#include "core/Paths.hpp"
#include "raylib.h"
#include "raymath.h"
#include <utility>
#include <rlgl.h>

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
    camera_.snapTo(car_.getPosition(), car_.getAngle());
    car_.loadSprite(assetPath("Cars/Car1.png"));
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
        camera_.snapTo(car_.getPosition(), car_.getAngle());
        lapTimer_.reset();
        resetRequested_ = false;
    }

    // La surface sous la voiture fixe la resistance au roulement
    surface_ = track_.surfaceAt(car_.getPosition());
    car_.setRollingCoef(Track::rollingCoeff(surface_));

    const CarInput& input = input_.step(dt);
    car_.update(dt, input);
    
    // Race started when car starts moving
    if (!raceStarted_ && std::fabs(car_.getLongitudinalSpeed()) > 0.1f) {
        raceStarted_ = true;
    } else { // Don't check for line crossing before race actually starts
        lineCrossed_ = track_.crossedFinishLine(car_.getPrevPos(), car_.getPosition());
    }

    lapTimer_.update(dt, lineCrossed_, raceStarted_);

    lineCrossed_ = false;  // reset for next frame


    engineSound_.update(car_.getEngineRPM(), input.throttle);

    for (const Vector2& corner : car_.getCorners()) {
        if (track_.isWall(corner)) { car_.collide(); break; }
    }
}

void RaceScene::draw() {
    const float ppm = car_.getPixelsPerMeter(); 
    camera_.update(GetFrameTime(), car_.getPosition(), car_.getAngle(), car_.getLongitudinalSpeed());
    const Camera3D cam = camera_.get(zoom_);

    BeginMode3D(cam);
        rlDisableDepthTest();          // tout est a plat : l'ordre de dessin suffit
        rlDisableBackfaceCulling(); 
        track_.drawGround();
        track_.draw3D();
        car_.draw3D();
    EndMode3D();
    const Vector2 p = car_.getPosition();
    const Vector2 sp = GetWorldToScreen({p.x, 0.0f, p.y}, cam);   // cam = Camera3D de la frame

    const char* txt = TextFormat("%.0f km/h", std::fabs(car_.getLongitudinalSpeed()) * 3.6f);
    const int w = MeasureText(txt, 24);
    DrawText(txt, (int)sp.x - w / 2, (int)sp.y - 60, 24, RAYWHITE);   // 60 px au-dessus de la voiture

    // HUD en coordonnees ecran, apres EndMode2D
    DrawText("Backspace: retour au depart | Molette: zoom", 20, 20, 20, RAYWHITE);
    DrawText(TextFormat("Surface: %s | Zoom: %.2f", surfaceName(surface_), zoom_), 20, 45, 20, RAYWHITE);
    int g = car_.getCurrentGear();
    const char* label = (g == -1) ? "R" : (g == 0) ? "N" : TextFormat("%d", g);
    DrawText(label, GetScreenWidth() - 60, GetScreenHeight() - 40, 40, RAYWHITE);
    car_.drawHud();

    // Affichage du chrono
    lapTimer_.draw(GetScreenWidth() - 200, 20);
}