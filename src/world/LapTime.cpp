#include "LapTime.hpp"
#include "raylib.h"

void LapTimer::update(float dt, bool lapCompleted, bool started) {
    if (!started) {
        return; // Do not update if the timer hasn't started
    }

    currentLapTime_ += dt;
    if (lapCompleted) {
        lapTimes_.push_back(currentLapTime_);
        currentLapTime_ = 0.0f;
    }
}

int LapTimer::getBestLapIndex() const {
    if (lapTimes_.empty()) {
        return -1; // No laps recorded
    }

    int bestLapIndex = 0;
    float bestLapTime = lapTimes_[0];

    for (size_t i = 1; i < lapTimes_.size(); ++i) {
        if (lapTimes_[i] < bestLapTime) {
            bestLapTime = lapTimes_[i];
            bestLapIndex = static_cast<int>(i);
        }
    }

    return bestLapIndex;
}

float LapTimer::getBestLapTime() const {
    int bestLapIndex = getBestLapIndex();
    if (bestLapIndex == -1) {
        return -1.0f; // No laps recorded
    }
    return lapTimes_[bestLapIndex];
}

void LapTimer::reset() {
    lapTimes_.clear();
    currentLapTime_ = 0.0f;
}

void LapTimer::draw(int x, int y) const {
    const char* currentLapLabel = TextFormat("Current Lap: %.2f s", currentLapTime_);
    DrawText(currentLapLabel, x, y, 20, RAYWHITE);

    if (!lapTimes_.empty()) {
        const char* lastLapLabel = TextFormat("Last Lap: %.2f s", lapTimes_.back());
        DrawText(lastLapLabel, x, y + 30, 20, RAYWHITE);
    }

    if (getBestLapIndex() != -1) {
        const char* bestLapLabel = TextFormat("Best Lap: %.2f s", getBestLapTime());
        DrawText(bestLapLabel, x, y + 60, 20, RAYWHITE);
    }
}