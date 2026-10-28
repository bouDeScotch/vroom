// src/core/Game.hpp
#pragma once
#include <memory>
#include <utility>
#include "Window.hpp"
#include "Assets.hpp"
#include "Scene.hpp"

class Game {
public:
    Game();
    void run();
    void quit() { running_ = false; }

    Assets& assets() { return assets_; }

    template <typename T, typename... Args>
    void changeScene(Args&&... args) {
        pending_ = std::make_unique<T>(*this, std::forward<Args>(args)...);
    }

private:
    Window window_;
    Assets assets_;
    std::unique_ptr<Scene> scene_;
    std::unique_ptr<Scene> pending_;
    bool running_ = true;

    static constexpr float kStep = 1.0f / 120.0f;
};