#pragma once

#include <vector>
#include <cstddef>

class LapTimer {
public:
    LapTimer() = default;

    void reset();

    int getLapCount() const { return static_cast<int>(lapTimes_.size()); }
    int getBestLapIndex() const;
    int getCurrentLapIndex() const { return getLapCount() - 1; }
    float getCurrentLapTime() const { return currentLapTime_; }
    int getLastLapIndex() const { return getLapCount() - 1; }
    float getLastLapTime() const {
        if (lapTimes_.empty()) {
            return -1.0f; // No laps recorded
        }
        return lapTimes_.back();
    }
    float getBestLapTime() const;
    void update(float dt, bool lapCompleted, bool started);
private:
    std::vector<float> lapTimes_;
    float currentLapTime_ = 0.0f;
};