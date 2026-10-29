#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <raylib.h>

// Types de surface lus dans le masque du circuit.
enum class Surface : std::uint8_t { Asphalt, Gravel, Grass, Wall };

// Parametres du circuit, modifiables sans toucher au reste du code.
struct TrackConfig {
    float metersPerPixel = 0.5f;  // echelle de l'image (0.5 => 1 pixel = 0,5 m)
    float startAngleDeg  = 0.0f;  // 0 = vers la droite (+x), positif = sens horaire a l'ecran (y vers le bas)
};

// Circuit defini par une image: couleurs plates = surfaces, 1 pixel magenta = depart.
// Coordonnees monde en METRES, origine en haut a gauche de l'image, x vers la droite, y vers le bas.
class Track {
public:
    explicit Track(const std::string& imagePath, TrackConfig config = {});
    ~Track();

    Track(const Track&) = delete;
    Track& operator=(const Track&) = delete;
    Track(Track&&) = delete;
    Track& operator=(Track&&) = delete;

    // Surface sous un point (en metres). Hors de l'image => Wall.
    Surface surfaceAt(Vector2 posMeters) const;
    bool isWall(Vector2 posMeters) const { return surfaceAt(posMeters) == Surface::Wall; }

    // Coefficient de resistance au roulement (remplace le 0.015 de resistanceForce).
    static float rollingCoeff(Surface s);

    Vector2 startPosition() const { return startPos_; }  // metres
    float startAngleRad() const;                         // radians
    Vector2 sizeMeters() const;

    // A appeler entre BeginMode2D et EndMode2D.
    void draw(float pixelsPerMeter) const;

private:
    TrackConfig config_;
    int width_  = 0;
    int height_ = 0;
    std::vector<std::uint8_t> surfaces_;  // 1 octet par pixel, valeurs de Surface
    Vector2 startPos_{};
    Texture2D texture_{};
};