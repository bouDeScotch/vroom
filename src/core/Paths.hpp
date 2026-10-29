// src/core/Paths.hpp
#pragma once
#include "raylib.h"
#include <string>

// Chemin d'une ressource, relatif au dossier de l'executable (independant du dossier courant).
inline std::string assetPath(const std::string& relative = "") {
    return std::string(GetApplicationDirectory()) + "assets/" + relative;
}
