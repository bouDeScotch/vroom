#pragma once
// Synthese procedurale du son d'un moteur 4 temps. Aucune dependance a raylib.
// setState() : thread principal. render() : thread audio.

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>

class EngineSynth {
public:
    struct Config {
        int cylinders = 4;
        std::array<int, 8> firingOrder{0, 2, 1, 3, 0, 0, 0, 0};  // boxer Subaru : ordre d'allumage 1-3-2-4 (indices 0-based)
        float resonanceHz = 110.0f;   // resonance de l'echappement : donne le "corps" du son
        float volume = 0.5f;
    };

    EngineSynth() : EngineSynth(44100.0f, Config{}) {}
    explicit EngineSynth(float sampleRate) : EngineSynth(sampleRate, Config{}) {}
    EngineSynth(float sampleRate, Config cfg)
        : cfg_(cfg), sr_(sampleRate), invSr_(1.0f / sampleRate) {
        // Chaque cylindre a une amplitude et une resonance legerement differentes.
        // Ces ecarts creent les sous-harmoniques du cycle (rpm/120) qui rendent le son vivant.
        std::uint32_t seed = 12345u;
        auto rnd = [&seed]() {  // [-1, 1]
            seed = seed * 1664525u + 1013904223u;
            return static_cast<float>((seed >> 8) & 0xFFFF) / 32768.0f - 1.0f;
        };
        for (int c = 0; c < kMaxCyl; ++c) {
            cylGain_[c] = 1.0f + 0.2f * rnd();
            cylFreq_[c] = 1.0f + 0.1f * rnd();
        }
        rpmSmooth_ = 1.0f - std::exp(-1.0f * invSr_ / 0.04f);
        thrSmooth_ = 1.0f - std::exp(-1.0f * invSr_ / 0.06f);
    }

    // rpm: regime moteur. throttle: 0 a 1.
    void setState(float rpm, float throttle) {
        rpm_.store(rpm, std::memory_order_relaxed);
        throttle_.store(throttle, std::memory_order_relaxed);
    }

    // Remplit `frames` echantillons mono (float, -1 a 1).
    void render(float* out, int frames) {
        const float rpmTarget = rpm_.load(std::memory_order_relaxed);
        const float thrTarget = std::clamp(throttle_.load(std::memory_order_relaxed), 0.0f, 1.0f);
        const float halfCyl = 0.5f * static_cast<float>(cfg_.cylinders);

        for (int i = 0; i < frames; ++i) {
            rpmS_ += (rpmTarget - rpmS_) * rpmSmooth_;
            thrS_ += (thrTarget - thrS_) * thrSmooth_;

            // 4 temps : cylinders/2 allumages par tour
            const float fireHz = std::max(rpmS_, 0.0f) / 60.0f * halfCyl;
            phase_ += fireHz * invSr_;
            if (phase_ >= 1.0f) {
                phase_ -= 1.0f;
                trigger(fireHz);
            }

            float s = 0.0f;
            for (Voice& v : voices_) {
                if (v.active) s += v.step(noise());
            }

            // Bruit d'admission/mecanique : monte avec la charge et le regime
            const float n = noise();
            noiseLp_ += 0.08f * (n - noiseLp_);
            s += noiseLp_ * 0.5f * thrS_ * (rpmS_ / 6000.0f);

            // Timbre : plus sombre sans gaz, plus brillant a pleine charge
            const float cutoff = 700.0f + 4300.0f * thrS_;
            const float a = 1.0f - std::exp(-6.2831853f * cutoff * invSr_);
            lp_ += a * (s - lp_);

            // Saturation douce (grondement a charge elevee)
            const float drive = 1.0f + 1.5f * thrS_;
            const float shaped = std::tanh(lp_ * drive) / std::tanh(drive);

            // Moteur coupe/cale : fondu vers le silence
            const float running = std::clamp((rpmS_ - 150.0f) / 250.0f, 0.0f, 1.0f);
            out[i] = shaped * cfg_.volume * running;
        }
    }

private:
    static constexpr int kMaxCyl = 8;
    static constexpr int kMaxVoices = 24;

    struct Voice {
        bool active = false;
        float env = 0, decay = 0, amp = 0;
        float ph1 = 0, ph2 = 0, inc1 = 0, inc2 = 0;
        float nEnv = 0, nDecay = 0, nAmp = 0;

        float step(float noise) {
            const float s = env * amp * (std::sin(ph1) + 0.45f * std::sin(ph2)) + nAmp * nEnv * noise;
            ph1 += inc1;
            ph2 += inc2;
            env *= decay;
            nEnv *= nDecay;
            if (env < 0.003f) active = false;
            return s;
        }
    };

    void trigger(float fireHz) {
        const int c = cfg_.firingOrder[orderIdx_ % cfg_.cylinders];
        ++orderIdx_;

        // Une voix libre, sinon on remplace la plus faible
        Voice* v = nullptr;
        for (Voice& x : voices_) if (!x.active) { v = &x; break; }
        if (!v) {
            v = &voices_[0];
            for (Voice& x : voices_) if (x.env < v->env) v = &x;
        }

        const float period = 1.0f / std::max(fireHz, 1.0f);
        const float tau = std::clamp(0.35f * period, 0.0015f, 0.015f);
        const float f0 = cfg_.resonanceHz * cylFreq_[c % kMaxCyl];
        const float jitter = 1.0f + 0.06f * noise();   // petite variation d'un allumage a l'autre

        v->active = true;
        v->env = 1.0f;
        v->decay = std::exp(-invSr_ / tau);
        v->amp = (0.3f + 0.7f * thrS_) * cylGain_[c % kMaxCyl] * jitter;
        v->ph1 = 0.0f;
        v->ph2 = 0.0f;
        v->inc1 = 6.2831853f * f0 * invSr_;
        v->inc2 = 6.2831853f * f0 * 2.3f * invSr_;
        v->nEnv = 1.0f;
        v->nDecay = std::exp(-invSr_ / (0.3f * tau));
        v->nAmp = 0.35f * thrS_;                      // claquement de l'echappement
    }

    float noise() {
        rng_ ^= rng_ << 13; rng_ ^= rng_ >> 17; rng_ ^= rng_ << 5;
        return static_cast<float>(rng_ & 0xFFFF) / 32768.0f - 1.0f;
    }

    Config cfg_;
    float sr_, invSr_;
    float rpmSmooth_ = 0, thrSmooth_ = 0;
    std::array<float, kMaxCyl> cylGain_{}, cylFreq_{};
    std::array<Voice, kMaxVoices> voices_{};

    std::atomic<float> rpm_{0.0f};
    std::atomic<float> throttle_{0.0f};

    float rpmS_ = 0, thrS_ = 0;
    float phase_ = 0;
    int orderIdx_ = 0;
    float noiseLp_ = 0, lp_ = 0;
    std::uint32_t rng_ = 2463534242u;
};