#pragma once

#include "rf/Types.h"

// Sound playback for synthesised 16-bit mono clips. The game generates every
// sound at start-up, so the engine only needs "keep this buffer, play it".
//
// Backend to write (phase 4 of docs/PLAN.md): XAudio2. One mastering voice,
// one source voice per sound; Play() submits the buffer, Stop() flushes it,
// Playing() asks the voice for its queued-buffer count. Until then
// engine/src/stubs/Audio_Stub.cpp accepts everything and stays silent.
namespace rf::audio {

constexpr int kMaxSounds = 32;

bool    Init();
void    Shutdown();
bool    Ready();

SoundId Create(const short* pcm16Mono, int frames, int sampleRate);   // copies the samples
void    Destroy(SoundId id);
int     Frames(SoundId id);
void    Play(SoundId id);            // restart from the beginning
void    Stop(SoundId id);
bool    Playing(SoundId id);
void    SetVolume(SoundId id, float volume);   // 0..1

} // namespace rf::audio
