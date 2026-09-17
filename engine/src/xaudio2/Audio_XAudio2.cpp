// XAudio2 backend: one mastering voice, one source voice per sound. The game
// synthesises every clip at start-up as 16-bit mono, so each Create() copies
// the samples once and Play() resubmits the same buffer. XAudio2 2.9 ships in
// Windows 10 and on the console; nothing to redistribute.
#include "rf/Audio.h"

#include <array>
#include <cassert>
#include <cstring>
#include <memory>

#undef NOGDI   // mmreg.h, pulled in by xaudio2.h, needs BITMAPINFOHEADER; nothing here is named Rectangle
#include <windows.h>
#include <xaudio2.h>
#include <wrl/client.h>

#include "rf/Log.h"

using Microsoft::WRL::ComPtr;

namespace rf::audio {

namespace {

struct Slot {
    IXAudio2SourceVoice*     voice {nullptr};   // owned: destroyed with DestroyVoice
    std::unique_ptr<short[]> samples;           // allocated at load, which is start-up for this game
    int  frames {0};
    bool used {false};
};

struct State {
    ComPtr<IXAudio2>        xaudio;
    IXAudio2MasteringVoice* master {nullptr};
    std::array<Slot, kMaxSounds> slots {};
    bool ready {false};
    bool comInitialised {false};
};

State g;

bool ValidSlot(SoundId id)
{
    return g.ready && id.index >= 0 && id.index < kMaxSounds && g.slots[static_cast<size_t>(id.index)].used;
}

Slot& At(SoundId id)
{
    assert(ValidSlot(id));
    return g.slots[static_cast<size_t>(id.index)];
}

void Release(Slot& s)
{
    if (s.voice != nullptr) {
        (void)s.voice->Stop(0, XAUDIO2_COMMIT_NOW);
        s.voice->DestroyVoice();
    }
    s = Slot {};
}

} // namespace

bool Init()
{
    assert(!g.ready);
    const HRESULT co = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    g.comInitialised = SUCCEEDED(co);   // RPC_E_CHANGED_MODE means someone else already did; fine

    HRESULT hr = XAudio2Create(&g.xaudio, 0, XAUDIO2_DEFAULT_PROCESSOR);
    if (FAILED(hr)) { log::Write(log::Level::Error, "AUDIO: XAudio2Create failed (0x%08lx)", hr); return false; }
    hr = g.xaudio->CreateMasteringVoice(&g.master);
    if (FAILED(hr)) { log::Write(log::Level::Error, "AUDIO: CreateMasteringVoice failed (0x%08lx)", hr); g.xaudio.Reset(); return false; }
    g.ready = true;
    log::Write(log::Level::Info, "AUDIO: XAudio2 ready");
    return true;
}

void Shutdown()
{
    for (Slot& s : g.slots) { Release(s); }
    if (g.master != nullptr) { g.master->DestroyVoice(); g.master = nullptr; }
    g.xaudio.Reset();
    if (g.comInitialised) { CoUninitialize(); }
    g.ready = false;
    g.comInitialised = false;
}

bool Ready()
{
    return g.ready;
}

SoundId Create(const short* pcm16Mono, int frames, int sampleRate)
{
    assert(pcm16Mono != nullptr && frames > 0 && sampleRate > 0);
    if (!g.ready) { return SoundId {}; }
    int slot = -1;
    for (int i = 0; i < kMaxSounds && slot < 0; ++i) { if (!g.slots[static_cast<size_t>(i)].used) { slot = i; } }
    if (slot < 0) { log::Write(log::Level::Warning, "AUDIO: sound table full (%d)", kMaxSounds); return SoundId {}; }

    WAVEFORMATEX wf {};
    wf.wFormatTag      = WAVE_FORMAT_PCM;
    wf.nChannels       = 1;
    wf.nSamplesPerSec  = static_cast<DWORD>(sampleRate);
    wf.wBitsPerSample  = 16;
    wf.nBlockAlign     = 2;
    wf.nAvgBytesPerSec = wf.nSamplesPerSec * wf.nBlockAlign;

    Slot& s = g.slots[static_cast<size_t>(slot)];
    const HRESULT hr = g.xaudio->CreateSourceVoice(&s.voice, &wf);
    if (FAILED(hr)) { log::Write(log::Level::Error, "AUDIO: CreateSourceVoice failed (0x%08lx)", hr); s = Slot {}; return SoundId {}; }
    s.samples = std::make_unique<short[]>(static_cast<size_t>(frames));
    std::memcpy(s.samples.get(), pcm16Mono, static_cast<size_t>(frames) * sizeof(short));
    s.frames = frames;
    s.used   = true;
    return SoundId {slot};
}

void Destroy(SoundId id)
{
    if (!ValidSlot(id)) { return; }
    Release(At(id));
}

int Frames(SoundId id)
{
    return ValidSlot(id) ? At(id).frames : 0;
}

void Play(SoundId id)
{
    if (!ValidSlot(id)) { return; }
    Slot& s = At(id);
    (void)s.voice->Stop(0, XAUDIO2_COMMIT_NOW);
    (void)s.voice->FlushSourceBuffers();
    XAUDIO2_BUFFER buf {};
    buf.Flags      = XAUDIO2_END_OF_STREAM;
    buf.AudioBytes = static_cast<UINT32>(s.frames) * sizeof(short);
    buf.pAudioData = reinterpret_cast<const BYTE*>(s.samples.get());
    if (SUCCEEDED(s.voice->SubmitSourceBuffer(&buf))) { (void)s.voice->Start(0, XAUDIO2_COMMIT_NOW); }
}

void Stop(SoundId id)
{
    if (!ValidSlot(id)) { return; }
    Slot& s = At(id);
    (void)s.voice->Stop(0, XAUDIO2_COMMIT_NOW);
    (void)s.voice->FlushSourceBuffers();
}

bool Playing(SoundId id)
{
    if (!ValidSlot(id)) { return false; }
    XAUDIO2_VOICE_STATE st {};
    At(id).voice->GetState(&st, XAUDIO2_VOICE_NOSAMPLESPLAYED);
    return st.BuffersQueued > 0;
}

void SetVolume(SoundId id, float volume)
{
    assert(volume >= 0.0f && volume <= 1.0f);
    if (!ValidSlot(id)) { return; }
    (void)At(id).voice->SetVolume(volume, XAUDIO2_COMMIT_NOW);
}

} // namespace rf::audio
