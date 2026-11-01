// src/scenes/CarSelectScene.cpp
#include "scenes/CarSelectScene.hpp"
#include "../core/Game.hpp"
#include "../entities/CarCatalog.hpp"
#include "MenuScene.hpp"
#include "raylib.h"
#include <algorithm>

namespace {

const char* driveLabel(float frontShare) {
    if (frontShare >= 0.99f) return "Traction";
    if (frontShare <= 0.01f) return "Propulsion";
    return "Transmission integrale";
}

// Barre de caracteristique : libelle a gauche, valeur a droite, remplissage = ratio (0..1) par rapport au meilleur du catalogue
void drawStatBar(int x, int y, int w, const char* label, const char* value, float ratio) {
    DrawText(label, x, y, 20, GRAY);
    DrawText(value, x + w - MeasureText(value, 20), y, 20, RAYWHITE);
    DrawRectangle(x, y + 26, w, 8, Color{60, 60, 70, 255});
    DrawRectangle(x, y + 26, static_cast<int>(w * std::clamp(ratio, 0.0f, 1.0f)), 8, YELLOW);
}

}  // namespace

CarSelectScene::CarSelectScene(Game& game, int initialIndex) : Scene(game) {
    const std::vector<CarModel>& cars = carCatalog();
    selected_ = std::clamp(initialIndex, 0, static_cast<int>(cars.size()) - 1);

    constexpr float kRpmToRad = 2.0f * PI / 60.0f;
    for (const CarModel& m : cars) {
        const Engine engine(m.engine);
        Stats s;
        for (float rpm = 500.0f; rpm <= m.engine.maxRPM; rpm += 25.0f) {
            const float torque = engine.fullThrottleTorque(rpm);
            s.peakTorque = std::max(s.peakTorque, torque);
            s.powerKw = std::max(s.powerKw, torque * rpm * kRpmToRad / 1000.0f);
        }
        const float radius = m.spec.wheelDiameter * 0.5f;
        for (int gear = 1; gear <= m.spec.maxGear; ++gear) {
            const float ratio = m.spec.gearRatios[gear + 1] * m.spec.finalDrive;  // 0 = R, 1 = N
            for (float rpm = 500.0f; rpm <= m.engine.maxRPM; rpm += 25.0f) {
                const float v = rpm * kRpmToRad / ratio * radius;                       // m/s
                const float drive = engine.fullThrottleTorque(rpm) * ratio / radius;    // N
                const float resist = 0.5f * 1.2f * m.spec.CdA * v * v + m.spec.weight * 9.81f * 0.015f;
                if (drive >= resist) s.Vmax = std::max(s.Vmax, v * 3.6f);
            }
        }
        stats_.push_back(s);

        // Le sprite est reduit ~5x a l'affichage : mipmaps pour eviter le scintillement
        Texture2D t = game_.assets().texture(m.sprite);
        if (t.id != 0) {
            GenTextureMipmaps(&t);
            SetTextureFilter(t, TEXTURE_FILTER_TRILINEAR);
        }
    }
}

void CarSelectScene::handleInput() {
    const int n = static_cast<int>(carCatalog().size());
    if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_DOWN)) selected_ = (selected_ + 1) % n;
    if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_UP))    selected_ = (selected_ + n - 1) % n;

    if (IsKeyPressed(KEY_ENTER)) game_.changeScene<MenuScene>(selected_);
    if (IsKeyPressed(KEY_ESCAPE)) game_.quit();
}

void CarSelectScene::draw() {
    const std::vector<CarModel>& cars = carCatalog();
    const CarModel& car = cars[selected_];
    const Stats& stats = stats_[selected_];
    const int w = GetScreenWidth();
    const int h = GetScreenHeight();

    const char* title = "CHOISIS TA VOITURE";
    DrawText(title, (w - MeasureText(title, 40)) / 2, 40, 40, RAYWHITE);

    // Voiture a gauche, vue de dessus, nez vers la droite, a l'echelle relative des autres voitures
    constexpr float kPixelsPerMeter = 60.0f;
    const Texture2D tex = game_.assets().texture(car.sprite);
    const float cx = w * 0.30f;
    const float cy = h * 0.50f;
    const float length = car.spec.size.x * kPixelsPerMeter;
    if (tex.id != 0) {
        const float scale = length / tex.height;
        const float tw = tex.width * scale;
        const float th = tex.height * scale;
        DrawTexturePro(tex, Rectangle{0, 0, static_cast<float>(tex.width), static_cast<float>(tex.height)},
                       Rectangle{cx, cy, tw, th}, Vector2{tw * 0.5f, th * 0.5f}, 90.0f, WHITE);
    } else {
        DrawRectanglePro(Rectangle{cx, cy, length, car.spec.size.y * kPixelsPerMeter},
                         Vector2{length * 0.5f, car.spec.size.y * kPixelsPerMeter * 0.5f}, 0.0f, BLUE);
    }

    const int nameSize = 36;
    DrawText(car.name.c_str(), static_cast<int>(cx) - MeasureText(car.name.c_str(), nameSize) / 2,
             static_cast<int>(cy) + 190, nameSize, YELLOW);

    // Caracteristiques a droite, barres relatives au meilleur de la liste
    float maxPower = 0.0f, maxTorque = 0.0f, maxMass = 0.0f, maxVmax = 0.0f;
    for (std::size_t i = 0; i < cars.size(); ++i) {
        maxPower = std::max(maxPower, stats_[i].powerKw);
        maxTorque = std::max(maxTorque, stats_[i].peakTorque);
        maxMass = std::max(maxMass, cars[i].spec.weight);
        maxVmax = std::max(maxVmax, stats_[i].Vmax);
    }

    const int px = static_cast<int>(w * 0.58f);
    const int pw = static_cast<int>(w * 0.34f);
    int py = static_cast<int>(h * 0.30f);
    drawStatBar(px, py, pw, "Puissance", TextFormat("%.0f kW (%.0f ch)", stats.powerKw, stats.powerKw * 1.36f),
                stats.powerKw / maxPower);
    py += 60;
    drawStatBar(px, py, pw, "Couple", TextFormat("%.0f Nm", stats.peakTorque), stats.peakTorque / maxTorque);
    py += 60;
    drawStatBar(px, py, pw, "Masse", TextFormat("%.0f kg", car.spec.weight), car.spec.weight / maxMass);
    py += 60;
    drawStatBar(px, py, pw, "Vitesse max", TextFormat("%.0f km/h", stats.Vmax), stats.Vmax / maxVmax);
    py += 70;
    DrawText(TextFormat("Boite : %d rapports", car.spec.maxGear), px, py, 22, RAYWHITE);
    DrawText(TextFormat("Transmission : %s", driveLabel(car.spec.driveFrontShare)), px, py + 30, 22, RAYWHITE);
    DrawText(TextFormat("Regime max : %.0f tr/min", car.engine.maxRPM), px, py + 60, 22, RAYWHITE);

    // Position dans la liste
    const int dot = 14, gap = 12;
    const int total = static_cast<int>(cars.size()) * (dot + gap) - gap;
    for (int i = 0; i < static_cast<int>(cars.size()); ++i) {
        DrawRectangle((w - total) / 2 + i * (dot + gap), h - 90, dot, dot, i == selected_ ? YELLOW : GRAY);
    }
    const char* hint = "Gauche / Droite : choisir     Entree : valider     Echap : quitter";
    DrawText(hint, (w - MeasureText(hint, 20)) / 2, h - 50, 20, GRAY);
}
