#pragma once

class Game;

class Scene {
public:
    explicit Scene(Game& game) : game_(game) {}
    virtual ~Scene() = default;

    virtual void update(float dt) = 0;
    virtual void draw() = 0;
    virtual void handleInput() {}
protected:
    Game& game_;
};