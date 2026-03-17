#pragma once
#include "IDocumentContent.h"
#include "core/parsers/gow2/SoundParser.h"
#include <string>
#include <memory>
#include <vector>
#include <cstdint>
#include <atomic>

namespace GOW {

class SoundPlayer : public IDocumentContent {
public:
    SoundPlayer(const std::string& name, std::unique_ptr<GOW2SoundParser::SoundBankData> bankData);
    SoundPlayer(const std::string& name, std::vector<int16_t> pcmData, uint32_t sampleRate, uint32_t channels = 1);
    ~SoundPlayer() override;

    std::string GetName() const override;
    void Draw() override;

    // Called by audio callback (free function in .cpp)
    void FillAudioBuffer(int16_t* output, uint32_t frameCount);

private:
    enum class PlayState { Stopped, Playing, Paused };

    std::string m_name;
    std::unique_ptr<GOW2SoundParser::SoundBankData> m_bankData;

    int m_selectedSound = -1;
    std::vector<int16_t> m_decodedPcm;
    uint32_t m_sampleRate = 22050;
    uint32_t m_channels = 1;
    bool m_singleSoundMode = false;

    PlayState m_state = PlayState::Stopped;
    std::atomic<size_t> m_playbackPos{0};

    // PIMPL for miniaudio device
    struct AudioDevice;
    std::unique_ptr<AudioDevice> m_audio;

    void DrawToolbar();
    void DrawSoundList();
    void DrawWaveform();

    void SelectSound(int index);
    void Play();
    void Pause();
    void Stop();
    void InitAudioDevice();
    void ShutdownAudioDevice();

};

} // namespace GOW
