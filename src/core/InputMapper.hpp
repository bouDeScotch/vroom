#pragma once

struct CarInput {
    float throttle = 0.0f; // 0.0f à 1.0f
    float brake = 0.0f;    // 0.0f à 1.0f
    float steering = 0.0f; // -1.0f à 1.0f
    float clutch = 0.0f;  // 0.0f à 1.0f
    bool handbrake = false;
    bool starter = false; // Maintenu pour lancer le demarreur
    bool shiftUp = false;
    bool shiftDown = false;
};

class InputMapper {
public:
    void poll();
    const CarInput& step(float dt);

private:
    CarInput raw_, smooth_;
    bool usingPad_ = false;
    int padIndex_ = -1;
    bool pendingShiftUp_ = false, pendingShiftDown_ = false;
};