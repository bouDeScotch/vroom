#pragma once
// Enveloppe raylib autour de EngineSynth : flux audio genere en continu par callback.
// Prerequis : InitAudioDevice() deja appele (apres InitWindow), CloseAudioDevice() a la fin.

#include "EngineSynth.hpp"
#include "raylib.h"

class EngineSound {
public:
    EngineSound();
    ~EngineSound();
    EngineSound(const EngineSound&) = delete;
    EngineSound& operator=(const EngineSound&) = delete;

    // A appeler a chaque pas de physique (ou chaque frame).
    void update(float rpm, float throttle) { synth_.setState(rpm, throttle); }

private:
    static void audioCallback(void* buffer, unsigned int frames);

    static constexpr unsigned int kSampleRate = 44100;
    static EngineSynth* active_;   // le callback raylib n'a pas de parametre utilisateur

    EngineSynth synth_;
    AudioStream stream_{};
};