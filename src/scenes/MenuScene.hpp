#pragma once
#include <functional>
#include <string>
#include <vector>
#include "../core/Scene.hpp"

class MenuScene : public Scene {
public:
    explicit MenuScene(Game& game);

    void handleInput() override;


    void update(float) {};
    void draw() override;

private:
    struct Item {
        std::string label;
        std::function<void()> action;
    };

    std::vector<Item> items_;
    int selected_ = 0;
};