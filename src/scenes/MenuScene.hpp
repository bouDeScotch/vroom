#pragma once
#include <functional>
#include <string>
#include <vector>
#include "../core/Scene.hpp"

class MenuScene : public Scene {
public:
    // carIndex : voiture choisie dans CarSelectScene (indice dans carCatalog())
    MenuScene(Game& game, int carIndex);

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
    int carIndex_ = 0;
};