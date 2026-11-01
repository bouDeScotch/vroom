#include "Track.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <rlgl.h>

namespace {

// Les valeurs 0..3 d'un circuit compressé (.trk) sont celles de Surface, voir tools/compress_assets.py
constexpr std::uint8_t kStartCode = 4;  // pixel de depart (magenta dans l'image d'origine)

Color displayColor(Surface s) {
    switch (s) {
        case Surface::Asphalt: return Color{128, 128, 128, 255};
        case Surface::Gravel:  return Color{210, 190, 130, 255};
        case Surface::Grass:   return Color{0, 160, 0, 255};
        case Surface::Wall:    return Color{200, 0, 0, 255};
    }
    return Color{0, 0, 0, 255};
}

struct RleImage {
    int width = 0;
    int height = 0;
    std::vector<std::uint8_t> pixels;  // une valeur par pixel, ligne par ligne
};

// Fichier .trk / .cpt (little-endian) : uint16 largeur, uint16 hauteur, puis des runs
// (uint8 valeur, uint24 repetitions) qui couvrent exactement largeur * hauteur pixels.
RleImage readRle(const std::string& path) {
    int size = 0;
    unsigned char* data = LoadFileData(path.c_str(), &size);
    if (data == nullptr) {
        throw std::runtime_error("Track: impossible de lire " + path);
    }
    struct Guard {
        unsigned char* p;
        ~Guard() { UnloadFileData(p); }
    } guard{data};

    if (size < 4 || (size - 4) % 4 != 0) {
        throw std::runtime_error("Track: fichier invalide (taille) " + path);
    }

    RleImage img;
    img.width = data[0] | (data[1] << 8);
    img.height = data[2] | (data[3] << 8);
    const std::size_t total = static_cast<std::size_t>(img.width) * static_cast<std::size_t>(img.height);
    img.pixels.resize(total);

    std::size_t filled = 0;
    for (int i = 4; i < size; i += 4) {
        const std::size_t count = data[i + 1] | (data[i + 2] << 8) | (data[i + 3] << 16);
        if (count > total - filled) {
            throw std::runtime_error("Track: fichier invalide (trop de pixels) " + path);
        }
        std::fill_n(img.pixels.begin() + static_cast<std::ptrdiff_t>(filled), count, data[i]);
        filled += count;
    }
    if (filled != total) {
        throw std::runtime_error("Track: fichier invalide (pixels manquants) " + path);
    }
    return img;
}

}  // namespace

Track::Track(const std::string& trackPath, TrackConfig config, const std::string& checkpointsPath) : config_(config) {
    RleImage file = readRle(trackPath);
    width_ = file.width;
    height_ = file.height;
    surfaces_ = std::move(file.pixels);

    double sumX = 0.0, sumY = 0.0;
    long startCount = 0;
    for (int y = 0; y < height_; ++y) {
        for (int x = 0; x < width_; ++x) {
            std::uint8_t& s = surfaces_[static_cast<std::size_t>(y) * width_ + x];
            if (s == kStartCode) {
                sumX += x + 0.5;
                sumY += y + 0.5;
                ++startCount;
                s = static_cast<std::uint8_t>(Surface::Asphalt);  // le depart est sur la route
            } else if (s > static_cast<std::uint8_t>(Surface::Wall)) {
                throw std::runtime_error("Track: type de terrain inconnu dans " + trackPath);
            }
        }
    }
    if (startCount == 0) {
        throw std::runtime_error("Track: aucun pixel de depart dans " + trackPath);
    }
    startPos_ = Vector2{static_cast<float>(sumX / startCount) * config_.metersPerPixel,
                        static_cast<float>(sumY / startCount) * config_.metersPerPixel};

    // Texture d'affichage construite a partir des surfaces (couleurs de palette uniquement).
    std::vector<unsigned char> rgba(surfaces_.size() * 4);
    for (std::size_t i = 0; i < surfaces_.size(); ++i) {
        const Color c = displayColor(static_cast<Surface>(surfaces_[i]));
        rgba[i * 4 + 0] = c.r;
        rgba[i * 4 + 1] = c.g;
        rgba[i * 4 + 2] = c.b;
        rgba[i * 4 + 3] = 255;
    }
    Image img{rgba.data(), width_, height_, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8};  // LoadTextureFromImage copie les pixels
    texture_ = LoadTextureFromImage(img);
    GenTextureMipmaps(&texture_);
    SetTextureFilter(texture_, TEXTURE_FILTER_TRILINEAR);
    SetTextureFilter(texture_, TEXTURE_FILTER_ANISOTROPIC_16X);

    if (!checkpointsPath.empty()) loadCheckpoints(checkpointsPath);
}

void Track::loadCheckpoints(const std::string& path) {
    RleImage file = readRle(path);
    if (file.width != width_ || file.height != height_) {
        throw std::runtime_error("Track: les checkpoints doivent avoir la meme taille que le circuit: " + path);
    }
    // 0 = aucun checkpoint, n = n-ieme checkpoint (1..N)
    checkpoints_ = std::move(file.pixels);
    checkpointCount_ = checkpoints_.empty() ? 0 : *std::max_element(checkpoints_.begin(), checkpoints_.end());
}

int Track::checkpointAt(Vector2 posMeters) const {
    if (checkpointCount_ == 0) return 0;
    const float fx = std::floor(posMeters.x / config_.metersPerPixel);
    const float fy = std::floor(posMeters.y / config_.metersPerPixel);
    if (!(fx >= 0.0f && fy >= 0.0f && fx < static_cast<float>(width_) && fy < static_cast<float>(height_))) {
        return 0;
    }
    return checkpoints_[static_cast<std::size_t>(fy) * width_ + static_cast<std::size_t>(fx)];
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

float Track::crossedFinishLine(Vector2 prevPos, Vector2 newPos) const {
    // Renvoie la fraction de la frame ou la ligne est franchie, -1 si pas franchie ou dans le mauvais sens.
    // On considere que la ligne de depart est perpendiculaire a l'angle de depart.
    // On calcule la distance signe par rapport a cette ligne pour les deux positions.
    const float angle = startAngleRad();
    const float cosA = std::cos(angle), sinA = std::sin(angle);
    const float dx = startPos_.x, dy = startPos_.y;
    const float dPrev = (prevPos.x - dx) * cosA + (prevPos.y - dy) * sinA;
    const float dNew  = (newPos.x  - dx) * cosA + (newPos.y  - dy) * sinA;
    // On doit verifier qu'on est bien dans la largeur de la ligne de depart (perpendiculaire a l'angle de depart).
    const float perpPrev = -(prevPos.x - dx) * sinA + (prevPos.y - dy) * cosA;
    const float perpNew  = -(newPos.x  - dx) * sinA + (newPos.y  - dy) * cosA;
    if (std::fabs(perpPrev) > config_.startLineWidth * 0.5f || std::fabs(perpNew) > config_.startLineWidth * 0.5f) {
        return -1.0f;
    }
    // Compter passage uniquement dans le bon sens
    if (dPrev >= 0.0f || dNew <= 0.0f) {
        return -1.0f;
    }
    // On a un passage de dPrev < 0 et dNew > 0, on calcule la fraction de la frame.
    return dPrev / (dPrev - dNew);
}