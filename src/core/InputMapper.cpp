#include "InputMapper.hpp"
#include "raylib.h"
#include <cmath>

namespace {
float moveTowards(float cur, float target, float maxDelta) {
    float d = target - cur;
    if (std::fabs(d) <= maxDelta) return target;
    return cur + (d > 0.0f ? maxDelta : -maxDelta);
}

float deadzone(float value, float deadzone = 0.1f) {
    if (std::fabs(value) < deadzone) return 0.0f;
    return (value - (value > 0.0f ? deadzone : -deadzone)) / (1.0f - deadzone);
}

// raylib envoie gachettes de -1.0f à 1.0f, on veut 0.0f à 1.0f
float normalizedTrigger(int pad, int axis) {
    return (GetGamepadAxisMovement(pad, axis) + 1.0f) / 2.0f;
}
}

void InputMapper::poll() {
    raw_ = {};
    usingPad_ = false;

    // Clavier (fleches et ZQSD) Note : raylib par du principe que les touches sont en QWERTY, donc ZQSD pour AZERTY devient WASD pour QWERTY
    raw_.throttle = IsKeyDown(KEY_W) ? 1.0f : 0.0f;
    raw_.brake = (IsKeyDown(KEY_S)) ? 1.0f : 0.0f;
    raw_.steering =   (float)(IsKeyDown(KEY_F)) 
                    - (float)(IsKeyDown(KEY_A));
    raw_.clutch = IsKeyDown(KEY_LEFT_CONTROL) ? 1.0f : 0.0f;
    raw_.handbrake = IsKeyDown(KEY_SPACE);
    raw_.starter = IsKeyDown(KEY_R);
    if (IsKeyPressed(KEY_UP)) pendingShiftUp_ = true;
    if (IsKeyPressed(KEY_DOWN)) pendingShiftDown_ = true;

    if (IsGamepadAvailable(padIndex_)) {
        float sx = deadzone(GetGamepadAxisMovement(padIndex_, GAMEPAD_AXIS_LEFT_X));
        float rt = normalizedTrigger(padIndex_, GAMEPAD_AXIS_RIGHT_TRIGGER);
        float lt = normalizedTrigger(padIndex_, GAMEPAD_AXIS_LEFT_TRIGGER);

        if (sx != 0.0f || rt != 0.0f || lt != 0.0f) {
            usingPad_ = true;
            raw_.throttle = rt;
            raw_.brake = lt;
            raw_.steering = sx;
        }
        raw_.clutch = IsGamepadButtonDown(padIndex_, GAMEPAD_BUTTON_LEFT_FACE_LEFT) ? 1.0f : 0.0f;
        raw_.handbrake = IsGamepadButtonDown(padIndex_, GAMEPAD_BUTTON_RIGHT_FACE_LEFT);
        raw_.starter = raw_.starter || IsGamepadButtonDown(padIndex_, GAMEPAD_BUTTON_MIDDLE_RIGHT);
        raw_.shiftUp = IsGamepadButtonDown(padIndex_, GAMEPAD_BUTTON_RIGHT_FACE_UP);
        raw_.shiftDown = IsGamepadButtonDown(padIndex_, GAMEPAD_BUTTON_RIGHT_FACE_DOWN);
    }
}

const CarInput& InputMapper::step(float dt) {
    if (usingPad_) {
        // Entrée analogiques deja lisses
        smooth_ = raw_;
        pendingShiftUp_ = pendingShiftDown_ = false;
        return smooth_;
    }

    // Clavier, on simule une progressivité avec retour a 0 plus rapide
    float steerRate = (raw_.steering == 0.0f) ? 5.0f : 4.0f;
    smooth_.throttle = moveTowards(smooth_.throttle, raw_.throttle, dt * 5.0f);
    smooth_.steering = moveTowards(smooth_.steering, raw_.steering, dt * steerRate);
    smooth_.brake = moveTowards(smooth_.brake, raw_.brake, dt * 8.0f);
    smooth_.clutch = moveTowards(smooth_.clutch, raw_.clutch, dt * 10.0f); // Rapide pour l'embrayage
    smooth_.handbrake = raw_.handbrake;
    smooth_.starter = raw_.starter;
    smooth_.shiftUp = pendingShiftUp_;
    smooth_.shiftDown = pendingShiftDown_;
    pendingShiftUp_ = pendingShiftDown_ = false;
    return smooth_;
}