#pragma once
#include <string>
#include <unordered_map>
#include "raylib.h"

class Assets {
public:
    explicit Assets(std::string root) : root_(std::move(root)) {}
    ~Assets() {
        for (auto& [_, t] : textures_) UnloadTexture(t);
        for (auto& [_, f] : fonts_) UnloadFont(f);
        for (auto& [_, s] : sounds_) UnloadSound(s);
    }
    Assets(const Assets&) = delete;
    Assets& operator=(const Assets&) = delete;

    const Texture2D& texture(const std::string& name) {
        auto it = textures_.find(name);
        if (it == textures_.end())
            it = textures_.emplace(name, LoadTexture((root_ + name).c_str())).first;
        return it->second;
    }
    const Sound& sound(const std::string& name) {
        auto it = sounds_.find(name);
        if (it == sounds_.end())
            it = sounds_.emplace(name, LoadSound((root_ + name).c_str())).first;
        return it->second;
    }
    const Font& font(const std::string& name) {
        auto it = fonts_.find(name);
        if (it == fonts_.end())
            it = fonts_.emplace(name, LoadFont((root_ + name).c_str())).first;
        return it->second;
    }
private:
    std::string root_;
    std::unordered_map<std::string, Texture2D> textures_;
    std::unordered_map<std::string, Sound> sounds_;
    std::unordered_map<std::string, Font> fonts_;
};