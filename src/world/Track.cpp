#include "Track.hpp"

#include <array>
#include <cmath>
#include <stdexcept>
#include <rlgl.h>

namespace {

struct PaletteEntry {
    Surface surface;
    int r, g, b;
};

// Ordre = priorite en cas d'egalite (le mur passe avant l'herbe pour ne jamais creer de trou).
constexpr std::array<PaletteEntry, 4> kPalette{{
    {Surface::Wall,    200,   0,   0},
    {Surface::Grass,     0, 160,   0},
    {Surface::Gravel,  210, 190, 130},
    {Surface::Asphalt, 128, 128, 128},
}};

constexpr int kStartR = 255, kStartG = 0, kStartB = 255;

Surface nearestSurface(int r, int g, int b) {
    int best = 0;
    long bestDist = -1;
    for (int i = 0; i < static_cast<int>(kPalette.size()); ++i) {
        const long dr = r - kPalette[i].r;
        const long dg = g - kPalette[i].g;
        const long db = b - kPalette[i].b;
        const long d = dr * dr + dg * dg + db * db;
        if (bestDist < 0 || d < bestDist) {
            bestDist = d;
            best = i;
        }
    }
    return kPalette[best].surface;
}

Color displayColor(Surface s) {
    switch (s) {
        case Surface::Asphalt: return Color{128, 128, 128, 255};
        case Surface::Gravel:  return Color{210, 190, 130, 255};
        case Surface::Grass:   return Color{0, 160, 0, 255};
        case Surface::Wall:    return Color{200, 0, 0, 255};
    }
    return Color{0, 0, 0, 255};
}

}  // namespace

Track::Track(const std::string& imagePath, TrackConfig config) : config_(config) {
    Image img = LoadImage(imagePath.c_str());
    if (img.data == nullptr) {
        throw std::runtime_error("Track: impossible de charger l'image " + imagePath);
    }
    ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);

    width_ = img.width;
    height_ = img.height;
    surfaces_.resize(static_cast<std::size_t>(width_) * static_cast<std::size_t>(height_));

    const auto* src = static_cast<const unsigned char*>(img.data);
    bool startFound = false;
    double sumX = 0.0, sumY = 0.0;
    long startCount = 0;

    for (int y = 0; y < height_; ++y) {
        for (int x = 0; x < width_; ++x) {
            const std::size_t i = static_cast<std::size_t>(y) * width_ + x;
            const int r = src[i * 4 + 0];
            const int g = src[i * 4 + 1];
            const int b = src[i * 4 + 2];

            if (r == kStartR && g == kStartG && b == kStartB) {
                sumX += x + 0.5;
                sumY += y + 0.5;
                ++startCount;
                startFound = true;
                surfaces_[i] = static_cast<std::uint8_t>(Surface::Asphalt);  // le depart est sur la route
            } else {
                surfaces_[i] = static_cast<std::uint8_t>(nearestSurface(r, g, b));
            }
        }
    }

    if (!startFound) {
        UnloadImage(img);
        throw std::runtime_error("Track: aucun pixel de depart magenta (255, 0, 255) dans " + imagePath);
    }
    startPos_ = Vector2{static_cast<float>(sumX / startCount) * config_.metersPerPixel,
                        static_cast<float>(sumY / startCount) * config_.metersPerPixel};

    // Texture d'affichage = image nettoyee (couleurs de palette uniquement).
    auto* dst = static_cast<unsigned char*>(img.data);
    for (std::size_t i = 0; i < surfaces_.size(); ++i) {
        const Color c = displayColor(static_cast<Surface>(surfaces_[i]));
        dst[i * 4 + 0] = c.r;
        dst[i * 4 + 1] = c.g;
        dst[i * 4 + 2] = c.b;
        dst[i * 4 + 3] = 255;
    }
    texture_ = LoadTextureFromImage(img);
    GenTextureMipmaps(&texture_);
    SetTextureFilter(texture_, TEXTURE_FILTER_ANISOTROPIC_16X);
    UnloadImage(img);
}

Track::~Track() {
    if (texture_.id != 0) {
        UnloadTexture(texture_);
    }
}

Surface Track::surfaceAt(Vector2 posMeters) const {
    const float fx = std::floor(posMeters.x / config_.metersPerPixel);
    const float fy = std::floor(posMeters.y / config_.metersPerPixel);
    // Ecrit ainsi pour que NaN et hors image donnent Wall.
    if (!(fx >= 0.0f && fy >= 0.0f && fx < static_cast<float>(width_) && fy < static_cast<float>(height_))) {
        return Surface::Wall;
    }
    const std::size_t i = static_cast<std::size_t>(fy) * width_ + static_cast<std::size_t>(fx);
    return static_cast<Surface>(surfaces_[i]);
}

float Track::rollingCoeff(Surface s) {
    switch (s) {
        case Surface::Asphalt: return 0.015f;
        case Surface::Gravel:  return 0.30f;
        case Surface::Grass:   return 0.12f;
        case Surface::Wall:    return 2.0f;  // la collision sera traitee a part mais on met un gros ralentissement pour essayer de ralentir
    }
    return 0.015f;
}

float Track::gripCoeff(Surface s) {
    switch (s) {
        case Surface::Asphalt: return 1.05f;
        case Surface::Gravel:  return 0.55f;
        case Surface::Grass:   return 0.45f;
        case Surface::Wall:    return 1.0f;
    }
    return 1.0f;
}

float Track::startAngleRad() const {
    return config_.startAngleDeg * DEG2RAD;
}

Vector2 Track::sizeMeters() const {
    return Vector2{width_ * config_.metersPerPixel, height_ * config_.metersPerPixel};
}

void Track::draw(float pixelsPerMeter) const {
    DrawTextureEx(texture_, Vector2{0.0f, 0.0f}, 0.0f, config_.metersPerPixel * pixelsPerMeter, WHITE);
}

void Track::drawGround() const {
    const float R = 20000.0f;                       // demi-taille en metres
    const Vector2 s = sizeMeters();
    const float cx = s.x * 0.5f, cz = s.y * 0.5f;

    rlSetTexture(0);
    rlBegin(RL_QUADS);
    rlColor4ub(0, 160, 0, 255);
    rlVertex3f(cx - R, 0.0f, cz - R);
    rlVertex3f(cx - R, 0.0f, cz + R);
    rlVertex3f(cx + R, 0.0f, cz + R);
    rlVertex3f(cx + R, 0.0f, cz - R);
    rlEnd();
}

void Track::draw3D() const {
    const Vector2 s = sizeMeters();
    rlSetTexture(texture_.id);
    rlBegin(RL_QUADS);
    rlColor4ub(255, 255, 255, 255);
    rlTexCoord2f(0, 0); rlVertex3f(0.0f, 0.0f, 0.0f);
    rlTexCoord2f(0, 1); rlVertex3f(0.0f, 0.0f, s.y);
    rlTexCoord2f(1, 1); rlVertex3f(s.x,  0.0f, s.y);
    rlTexCoord2f(1, 0); rlVertex3f(s.x,  0.0f, 0.0f);
    rlEnd();
    rlSetTexture(0);
}