#include "EngineSound.hpp"

#include <cstring>

EngineSynth* EngineSound::active_ = nullptr;

EngineSound::EngineSound() : synth_(static_cast<float>(kSampleRate)) {
    SetAudioStreamBufferSizeDefault(1024);               // ~23 ms de latence ; a appeler avant LoadAudioStream
    stream_ = LoadAudioStream(kSampleRate, 32, 1);       // 32 bits = float, mono
    active_ = &synth_;
    SetAudioStreamCallback(stream_, &EngineSound::audioCallback);
    PlayAudioStream(stream_);
}

EngineSound::~EngineSound() {
    SetAudioStreamCallback(stream_, nullptr);
    StopAudioStream(stream_);
    UnloadAudioStream(stream_);
    active_ = nullptr;
}

void EngineSound::audioCallback(void* buffer, unsigned int frames) {
    if (active_ == nullptr) {
        std::memset(buffer, 0, frames * sizeof(float));
        return;
    }
    active_->render(static_cast<float*>(buffer), static_cast<int>(frames));
}