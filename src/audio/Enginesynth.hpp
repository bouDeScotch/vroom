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
        int cylinders = 4;             // 16 maximum
        std::array<int, 16> firingOrder{0, 2, 1, 3};  // boxer Subaru : ordre d'allumage 1-3-2-4 (indices 0-based)
        float resonanceHz = 110.0f;   // resonance de l'echappement : donne le "corps" du son
        float volume = 0.5f;
        float brightness = 1.0f;      // echelle du filtre passe-bas : < 1 son plus sourd, > 1 plus mordant
        float damping = 1.0f;         // duree de chaque allumage : < 1 son plus sec et plus lisse, > 1 plus rond et gras
        float contrast = 0.35f;       // ecart entre sans gaz (0) et plein gaz (1) : 0 = aucun, 1 = le son disparait sans gaz
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

            // Les impulsions d'allumage sont toutes positives : sans filtrage de la composante continue, elle
            // s'accumule quand les allumages se chevauchent (beaucoup de cylindres, haut regime) et sature la sortie
            dc_ += 0.002f * (s - dc_);
            s -= dc_;

            // La charge percue ne tombe jamais a zero : un moteur sans gaz reste plein de son
            const float load = currentLoad();

            // Bruit d'admission/mecanique : monte avec la charge et le regime
            const float n = noise();
            noiseLp_ += 0.08f * (n - noiseLp_);
            s += noiseLp_ * 0.5f * load * (rpmS_ / 6000.0f) * voiceGain_;   // meme baisse que les allumages, sinon il domine a haut regime

            // Timbre : plus sombre a faible charge, plus brillant a pleine charge
            const float cutoff = (700.0f + 4300.0f * load) * cfg_.brightness;
            const float a = 1.0f - std::exp(-6.2831853f * cutoff * invSr_);
            lp_ += a * (s - lp_);
            lp2_ += a * (lp_ - lp2_);   // deuxieme pole : 12 dB/octave, les aigus des allumages rapides ne sifflent plus

            // Saturation douce (grondement a charge elevee)
            const float drive = 1.0f + 1.5f * load;
            const float shaped = std::tanh(lp2_ * drive) / std::tanh(drive);

            // Moteur coupe/cale : fondu vers le silence
            const float running = std::clamp((rpmS_ - 150.0f) / 250.0f, 0.0f, 1.0f);
            // Compense en partie la baisse de niveau par voix (sinon les moteurs a beaucoup de cylindres deviennent faibles a haut regime)
            const float makeup = 1.0f / std::sqrt(voiceGain_);
            out[i] = std::clamp(shaped * cfg_.volume * running * makeup, -1.0f, 1.0f);
        }
    }

private:
    static constexpr int kMaxCyl = 16;
    static constexpr int kMaxVoices = 48;

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

    float currentLoad() const { return (1.0f - cfg_.contrast) + cfg_.contrast * thrS_; }

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
        const float tau = std::clamp(0.35f * period, 0.0015f, 0.015f) * cfg_.damping;

        // Allumages simultanes (une voix dure ~5.8 tau). Plus il y en a, plus on baisse chaque voix pour que la somme
        // reste dans la meme plage, et plus on reduit les ecarts de frequence : des dizaines de sinusoides
        // desaccordees se melangent en bruit blanc, alors que des impulsions regulieres donnent une note.
        const float overlap = fireHz * 5.8f * tau;
        const float voiceGain = 1.0f / std::sqrt(std::max(1.0f, overlap * 0.5f));
        const float spread = std::min(1.0f, 6.0f / std::max(overlap, 1.0f));
        voiceGain_ = voiceGain;

        const float f0 = cfg_.resonanceHz * (1.0f + (cylFreq_[c % kMaxCyl] - 1.0f) * spread);
        const float jitter = 1.0f + 0.06f * spread * noise();   // petite variation d'un allumage a l'autre

        v->active = true;
        v->env = 1.0f;
        v->decay = std::exp(-invSr_ / tau);
        const float load = currentLoad();
        v->amp = (0.3f + 0.7f * load) * cylGain_[c % kMaxCyl] * jitter;   // multiplie plus bas par voiceGain
        v->amp *= voiceGain;
        v->ph1 = 0.0f;
        v->ph2 = 0.0f;
        v->inc1 = 6.2831853f * f0 * invSr_;
        v->inc2 = 6.2831853f * f0 * 2.3f * invSr_;
        v->nEnv = 1.0f;
        v->nDecay = std::exp(-invSr_ / (0.3f * tau));
        v->nAmp = 0.35f * load * voiceGain;           // claquement de l'echappement
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
    float noiseLp_ = 0, lp_ = 0, lp2_ = 0, dc_ = 0;
    float voiceGain_ = 1.0f;   // derniere baisse de niveau appliquee aux voix (thread audio uniquement)
    std::uint32_t rng_ = 2463534242u;
};