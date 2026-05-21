#include "SoundPlayer.h"
#include "core/Logger.h"
#include "core/ThemeManager.h"
#include "ui/Widgets.h"
#include "core/audio/AdpcmDecoder.h"
#include "fonts/SFSymbols.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <imgui.h>


#include "miniaudio.h"

namespace GOW {

float SoundPlayer::s_volume = 1.0f;

struct SoundPlayer::AudioDevice {
  ma_device device;
  bool initialized = false;
};

// ── Audio callback (runs on audio thread) ─────────────────────────────────

static void ma_data_callback(ma_device *pDevice, void *pOutput,
                             const void * /*pInput*/, ma_uint32 frameCount) {
  auto *player = (SoundPlayer *)pDevice->pUserData;
  if (player) {
    player->FillAudioBuffer((int16_t *)pOutput, (uint32_t)frameCount);
  } else {
    memset(pOutput, 0, frameCount * 2 * sizeof(int16_t));
  }
}

void SoundPlayer::FillAudioBuffer(int16_t *output, uint32_t frameCount) {
  uint32_t ch = m_channels;
  size_t samplesNeeded = frameCount * ch;

  if (m_state != PlayState::Playing || m_decodedPcm.empty()) {
    memset(output, 0, samplesNeeded * sizeof(int16_t));
    return;
  }

  size_t pos = m_playbackPos.load(std::memory_order_relaxed);
  size_t totalSamples = m_decodedPcm.size();

  float vol = s_volume;
  for (size_t i = 0; i < samplesNeeded; i++) {
    if (pos < totalSamples) {
      int32_t s = (int32_t)((float)m_decodedPcm[pos++] * vol);
      output[i] = (int16_t)(s < -32768 ? -32768 : s > 32767 ? 32767 : s);
    } else {
      output[i] = 0;
    }
  }

  m_playbackPos.store(pos, std::memory_order_relaxed);

  if (pos >= totalSamples) {
    m_state = PlayState::Stopped;
  }
}

// ── Constructor / Destructor ──────────────────────────────────────────────

SoundPlayer::SoundPlayer(
    const std::string &name,
    std::unique_ptr<GOW2SoundParser::SoundBankData> bankData)
    : m_name(name), m_bankData(std::move(bankData)),
      m_audio(std::make_unique<AudioDevice>()) {
  m_sampleRate = m_bankData ? m_bankData->defaultSampleRate : 22050;
  m_channels = 1;
  InitAudioDevice();

  // Auto-select first sound with data
  for (int i = 0; i < (int)m_bankData->sounds.size(); i++) {
    if (m_bankData->sounds[i].hasData) {
      SelectSound(i);
      break;
    }
  }
}

SoundPlayer::SoundPlayer(const std::string &name, std::vector<int16_t> pcmData,
                         uint32_t sampleRate, uint32_t channels)
    : m_name(name), m_audio(std::make_unique<AudioDevice>()),
      m_singleSoundMode(true) {
  m_sampleRate = sampleRate;
  m_channels = channels;
  m_decodedPcm = std::move(pcmData);
  m_selectedSound = 0;
  InitAudioDevice();
}

SoundPlayer::~SoundPlayer() { ShutdownAudioDevice(); }

std::string SoundPlayer::GetName() const { return m_name; }

// ── Audio device management ───────────────────────────────────────────────

void SoundPlayer::InitAudioDevice() {
  ma_device_config config = ma_device_config_init(ma_device_type_playback);
  config.playback.format = ma_format_s16;
  config.playback.channels = m_channels;
  config.sampleRate = m_sampleRate;
  config.dataCallback = ma_data_callback;
  config.pUserData = this;

  if (ma_device_init(NULL, &config, &m_audio->device) != MA_SUCCESS) {
    LOG_ERR("[SoundPlayer] Failed to initialize audio device");
    return;
  }
  m_audio->initialized = true;
}

void SoundPlayer::ShutdownAudioDevice() {
  Stop();
  if (m_audio && m_audio->initialized) {
    ma_device_uninit(&m_audio->device);
    m_audio->initialized = false;
  }
}

// ── Playback controls ─────────────────────────────────────────────────────

void SoundPlayer::SelectSound(int index) {
  if (index < 0 || index >= (int)m_bankData->sounds.size())
    return;

  Stop();
  m_selectedSound = index;

  const auto &snd = m_bankData->sounds[index];
  if (!snd.hasData || snd.adpcmSize == 0) {
    m_decodedPcm.clear();
    LOG_INFO("[SoundPlayer] Sound '%s' has no ADPCM data", snd.name.c_str());
    return;
  }

  // Bounds check
  if (snd.adpcmOffset + snd.adpcmSize > m_bankData->bankStreamData.size()) {
    m_decodedPcm.clear();
    LOG_ERR("[SoundPlayer] ADPCM data out of bounds for '%s': offset=0x%X "
            "size=0x%X streamSize=0x%zX",
            snd.name.c_str(), snd.adpcmOffset, snd.adpcmSize,
            m_bankData->bankStreamData.size());
    return;
  }

  m_decodedPcm = AdpcmDecoder::Decode(
      m_bankData->bankStreamData.data() + snd.adpcmOffset, snd.adpcmSize);

  LOG_INFO("[SoundPlayer] Decoded '%s': %zu PCM samples", snd.name.c_str(),
           m_decodedPcm.size());
}

void SoundPlayer::Play() {
  if (!m_audio || !m_audio->initialized)
    return;
  if (m_decodedPcm.empty())
    return;

  if (m_state == PlayState::Stopped) {
    m_playbackPos.store(0, std::memory_order_relaxed);
  }

  m_state = PlayState::Playing;
  ma_device_start(&m_audio->device);
}

void SoundPlayer::Pause() {
  if (m_state == PlayState::Playing) {
    m_state = PlayState::Paused;
    if (m_audio && m_audio->initialized)
      ma_device_stop(&m_audio->device);
  }
}

void SoundPlayer::Stop() {
  m_state = PlayState::Stopped;
  m_playbackPos.store(0, std::memory_order_relaxed);
  if (m_audio && m_audio->initialized)
    ma_device_stop(&m_audio->device);
}

// ── Drawing ───────────────────────────────────────────────────────────────

void SoundPlayer::Draw() {
  if (!m_bankData && !m_singleSoundMode) {
    ImGui::TextDisabled("No sound data");
    return;
  }

  DrawToolbar();
  ImGui::Separator();

  if (m_singleSoundMode) {
    // Single sound: just waveform, no list
    ImVec2 avail = ImGui::GetContentRegionAvail();
    ImGui::BeginChild("##waveform", ImVec2(0, avail.y), true);
    DrawWaveform();
    ImGui::EndChild();
  } else {
    ImVec2 avail = ImGui::GetContentRegionAvail();
    float listWidth = 200.0f;

    ImGui::BeginChild("##soundlist", ImVec2(listWidth, avail.y), true);
    DrawSoundList();
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("##waveform", ImVec2(0, avail.y), true);
    DrawWaveform();
    ImGui::EndChild();
  }
}

void SoundPlayer::DrawToolbar() {
  ImGui::PushStyleColor(ImGuiCol_Button, GOW::Theme::ToolbarButton());
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, GOW::Theme::ToolbarButtonHover());

  bool hasSound = !m_decodedPcm.empty();

  if (hasSound && m_state != PlayState::Playing) {
    if (GOW::UI::Widgets::SmallButton(ICON_SF_PLAY_FILL))
      Play();
  } else if (m_state == PlayState::Playing) {
    if (GOW::UI::Widgets::SmallButton(ICON_SF_PAUSE_FILL))
      Pause();
  } else {
    ImGui::BeginDisabled();
    ImGui::SmallButton(ICON_SF_PLAY_FILL);
    ImGui::EndDisabled();
  }

  ImGui::SameLine();
  if (hasSound && m_state != PlayState::Stopped) {
    if (GOW::UI::Widgets::SmallButton(ICON_SF_STOP_FILL))
      Stop();
  } else {
    ImGui::BeginDisabled();
    ImGui::SmallButton(ICON_SF_STOP_FILL);
    ImGui::EndDisabled();
  }

  ImGui::SameLine();
  ImGui::TextDisabled("|");
  ImGui::SameLine();

  if (m_singleSoundMode) {
    ImGui::TextDisabled("%s", m_name.c_str());
    ImGui::SameLine();
    if (!m_decodedPcm.empty()) {
      size_t pos = m_playbackPos.load(std::memory_order_relaxed);
      float sRate = (float)(m_sampleRate * m_channels);
      float seconds = (float)pos / sRate;
      float total = (float)m_decodedPcm.size() / sRate;
      ImGui::TextDisabled("%.1f / %.1fs", seconds, total);
    }
  } else if (m_selectedSound >= 0 && m_bankData &&
             m_selectedSound < (int)m_bankData->sounds.size()) {
    const auto &snd = m_bankData->sounds[m_selectedSound];
    ImGui::TextDisabled("%s", snd.name.c_str());
    ImGui::SameLine();

    if (!m_decodedPcm.empty()) {
      size_t pos = m_playbackPos.load(std::memory_order_relaxed);
      float seconds = (float)pos / (float)m_bankData->defaultSampleRate;
      float total =
          (float)m_decodedPcm.size() / (float)m_bankData->defaultSampleRate;
      ImGui::TextDisabled("%.1f / %.1fs", seconds, total);
    }
  } else {
    ImGui::TextDisabled("No sound selected");
  }

  ImGui::SameLine();
  ImGui::TextDisabled("|");
  ImGui::SameLine();
  float volPct = s_volume * 100.0f;
  ImGui::SetNextItemWidth(80.0f);
  if (ImGui::SliderFloat("##vol", &volPct, 0.0f, 100.0f, "%.0f%%"))
    s_volume = volPct / 100.0f;
  if (ImGui::IsItemHovered())
    ImGui::SetTooltip("Volume");

  ImGui::PopStyleColor(2);
}

void SoundPlayer::DrawSoundList() {
  ImGui::TextDisabled("Sounds (%zu)", m_bankData->sounds.size());
  ImGui::Separator();

  for (int i = 0; i < (int)m_bankData->sounds.size(); i++) {
    const auto &snd = m_bankData->sounds[i];
    bool selected = (i == m_selectedSound);

    ImGui::PushID(i);
    if (!snd.hasData) {
      ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
    }

    if (ImGui::Selectable(snd.name.c_str(), selected)) {
      SelectSound(i);
    }

    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
      SelectSound(i);
      Play();
    }

    if (!snd.hasData) {
      ImGui::PopStyleColor();
    }
    ImGui::PopID();
  }
}

void SoundPlayer::DrawWaveform() {
  if (m_decodedPcm.empty()) {
    ImGui::TextDisabled("Select a sound to view waveform");
    return;
  }

  ImVec2 avail = ImGui::GetContentRegionAvail();
  if (avail.x < 10 || avail.y < 10)
    return;

  // Downsample for display
  int displayWidth = (int)avail.x;
  int totalSamples = (int)m_decodedPcm.size();
  int samplesPerPixel = std::max(1, totalSamples / displayWidth);

  std::vector<float> displayData(displayWidth);
  for (int i = 0; i < displayWidth; i++) {
    int start = i * samplesPerPixel;
    int end = std::min(start + samplesPerPixel, totalSamples);
    float maxVal = 0.0f;
    for (int j = start; j < end; j++) {
      float val = (float)m_decodedPcm[j] / 32768.0f;
      if (std::abs(val) > std::abs(maxVal))
        maxVal = val;
    }
    displayData[i] = maxVal;
  }

  // Draw waveform
  ImGui::PlotLines("##waveform_plot", displayData.data(), displayWidth, 0,
                   nullptr, -1.0f, 1.0f, ImVec2(avail.x, avail.y - 20));

  // Draw playback cursor overlay
  if (m_state != PlayState::Stopped && totalSamples > 0) {
    size_t pos = m_playbackPos.load(std::memory_order_relaxed);
    float progress = (float)pos / (float)totalSamples;

    ImVec2 plotPos = ImGui::GetItemRectMin();
    ImVec2 plotSize = ImGui::GetItemRectSize();
    float cursorX = plotPos.x + progress * plotSize.x;

    ImDrawList *drawList = ImGui::GetWindowDrawList();
    drawList->AddLine(ImVec2(cursorX, plotPos.y),
                      ImVec2(cursorX, plotPos.y + plotSize.y),
                      IM_COL32(255, 165, 0, 200), 2.0f);
  }

  // Clickable seek
  if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(0) && totalSamples > 0) {
    ImVec2 plotPos = ImGui::GetItemRectMin();
    ImVec2 plotSize = ImGui::GetItemRectSize();
    float mouseX = ImGui::GetMousePos().x;
    float progress = (mouseX - plotPos.x) / plotSize.x;
    progress = std::clamp(progress, 0.0f, 1.0f);
    size_t newPos = (size_t)(progress * totalSamples);
    m_playbackPos.store(newPos, std::memory_order_relaxed);
  }
}

} // namespace GOW
