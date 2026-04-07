#include "ui/viewers/VideoPlayer.h"
#include "core/Logger.h"
#include "fonts/SFSymbols.h"
#include <algorithm>
#include <cmath>
#include <glad/glad.h>
#include <imgui.h>
#include <imgui_internal.h>

namespace GOW {

// ─── FFmpeg Custom IO ───────────────────────────────────────────────────────
static int avioRead(void *opaque, uint8_t *buf, int buf_size) {
  auto *ctx = static_cast<IFile *>(opaque);
  if (!ctx)
    return AVERROR_EOF;
  size_t r = ctx->Read(buf, buf_size);
  if (r == 0)
    return AVERROR_EOF;
  return (int)r;
}

static int64_t avioSeek(void *opaque, int64_t offset, int whence) {
  auto *ctx = static_cast<IFile *>(opaque);
  if (!ctx)
    return AVERROR_EOF;
  if (whence == AVSEEK_SIZE)
    return ctx->Size();

  // FFmpeg sometimes passes weird flags; mask them out for standard fseek
  whence &= ~AVSEEK_FORCE;

  if (ctx->Seek(offset, whence)) {
    return ctx->Tell();
  }
  return -1;
}

// ─── AudioRingBuffer ────────────────────────────────────────────────────────
void AudioRingBuffer::push(const uint8_t *data, size_t size) {
  if (size == 0)
    return;
  std::lock_guard<std::mutex> lock(m_mutex);
  if (m_size + size > m_capacity)
    return; // Drop if full

  size_t firstPart = std::min(size, m_capacity - m_writePos);
  std::memcpy(m_buffer.data() + m_writePos, data, firstPart);
  if (size > firstPart) {
    std::memcpy(m_buffer.data(), data + firstPart, size - firstPart);
  }
  m_writePos = (m_writePos + size) % m_capacity;
  m_size += size;
}

size_t AudioRingBuffer::pop(uint8_t *outData, size_t reqSize) {
  if (reqSize == 0)
    return 0;
  std::lock_guard<std::mutex> lock(m_mutex);
  size_t toCopy = std::min(m_size, reqSize);
  if (toCopy > 0) {
    size_t firstPart = std::min(toCopy, m_capacity - m_readPos);
    std::memcpy(outData, m_buffer.data() + m_readPos, firstPart);
    if (toCopy > firstPart) {
      std::memcpy(outData + firstPart, m_buffer.data(), toCopy - firstPart);
    }
    m_readPos = (m_readPos + toCopy) % m_capacity;
    m_size -= toCopy;
  }
  return toCopy;
}

void AudioRingBuffer::clear() {
  std::lock_guard<std::mutex> lock(m_mutex);
  m_writePos = 0;
  m_readPos = 0;
  m_size = 0;
}

size_t AudioRingBuffer::size() const {
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_size;
}

// ─── VideoPlayer Implementation ─────────────────────────────────────────────

VideoPlayer::VideoPlayer(const std::string &name, std::shared_ptr<IFile> file)
    : m_name(name), m_ifile(std::move(file)) {
  m_isOpen = FinishOpen();
  if (!m_isOpen) {
    LOG_ERR("[VideoPlayer] Initialization failed for %s", m_name.c_str());
    Close();
  }
}

VideoPlayer::~VideoPlayer() { Close(); }

bool VideoPlayer::FinishOpen() {
  if (!m_ifile || !m_ifile->IsValid())
    return false;
  m_fileSize = m_ifile->Size();
  m_ifile->Seek(0, SEEK_SET);

  // 1. Setup AVIO
  constexpr int kBufSize = 256 * 1024; // 256KB reduces I/O overhead
  uint8_t *avioBuf = static_cast<uint8_t *>(av_malloc(kBufSize));
  m_avioCtx = avio_alloc_context(avioBuf, kBufSize, 0, m_ifile.get(), avioRead,
                                 nullptr, avioSeek);
  if (!m_avioCtx) {
    av_free(avioBuf);
    return false;
  }

  // 2. Open Format Context
  m_fmtCtx = avformat_alloc_context();
  m_fmtCtx->pb = m_avioCtx;
  m_fmtCtx->flags |= AVFMT_FLAG_CUSTOM_IO;

  if (avformat_open_input(&m_fmtCtx, "custom_io", nullptr, nullptr) < 0) {
    LOG_ERR("[VideoPlayer] Failed to open input format");
    return false;
  }

  if (avformat_find_stream_info(m_fmtCtx, nullptr) < 0) {
    LOG_ERR("[VideoPlayer] Failed to find stream info");
    return false;
  }

  m_videoStream =
      av_find_best_stream(m_fmtCtx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
  m_audioStream = av_find_best_stream(m_fmtCtx, AVMEDIA_TYPE_AUDIO, -1,
                                      m_videoStream, nullptr, 0);

  if (m_videoStream < 0) {
    LOG_ERR("[VideoPlayer] No video stream found");
    return false;
  }

  // 3. Setup Video Codec (Multi-threaded software decode)
  const auto *vStream = m_fmtCtx->streams[m_videoStream];
  const auto *vCodec = avcodec_find_decoder(vStream->codecpar->codec_id);
  m_videoCtx = avcodec_alloc_context3(vCodec);
  avcodec_parameters_to_context(m_videoCtx, vStream->codecpar);
  m_videoCtx->thread_count = 0; // use all CPU cores
  m_videoCtx->thread_type = FF_THREAD_FRAME | FF_THREAD_SLICE;

  if (avcodec_open2(m_videoCtx, vCodec, nullptr) < 0) {
    LOG_ERR("[VideoPlayer] Failed to open video codec");
    return false;
  }

  m_videoCodecName = vCodec ? vCodec->name : "Unknown";
  m_videoWidth = vStream->codecpar->width;
  m_videoHeight = vStream->codecpar->height;

  m_videoTimeBase = av_q2d(vStream->time_base);
  if (vStream->avg_frame_rate.num > 0) {
    m_frameDuration = av_q2d(av_inv_q(vStream->avg_frame_rate));
    m_fps = av_q2d(vStream->avg_frame_rate);
  } else {
    m_frameDuration = 1.0 / 30.0;
    m_fps = 30.0;
  }

  // 4. Setup Audio Codec (If present)
  if (m_audioStream >= 0) {
    const auto *aStream = m_fmtCtx->streams[m_audioStream];
    const auto *aCodec = avcodec_find_decoder(aStream->codecpar->codec_id);
    if (aCodec) {
      m_audioCodecName = aCodec->name;
      m_audioCtx = avcodec_alloc_context3(aCodec);
      avcodec_parameters_to_context(m_audioCtx, aStream->codecpar);
      if (avcodec_open2(m_audioCtx, aCodec, nullptr) >= 0) {
        m_audioTimeBase = av_q2d(aStream->time_base);
        m_hasAudio = true;
      } else {
        LOG_ERR("[VideoPlayer] Failed to open audio codec, video only mode");
        m_hasAudio = false;
      }
    }
  }

  // 5. Setup SwrContext (Audio Resampling)
  if (m_hasAudio) {
    AVChannelLayout outLayout = AV_CHANNEL_LAYOUT_STEREO;
    m_audioChannels = outLayout.nb_channels;
    m_audioSampleRate = 44100; // Standardize Output

    swr_alloc_set_opts2(&m_swrCtx, &outLayout, AV_SAMPLE_FMT_FLT,
                        m_audioSampleRate, &m_audioCtx->ch_layout,
                        m_audioCtx->sample_fmt, m_audioCtx->sample_rate, 0,
                        nullptr);
    swr_init(m_swrCtx);

    // Init MiniAudio
    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format = ma_format_f32;
    config.playback.channels = m_audioChannels;
    config.sampleRate = m_audioSampleRate;
    config.dataCallback = AudioCallback;
    config.pUserData = this;

    if (ma_device_init(NULL, &config, &m_audioDevice) == MA_SUCCESS) {
      m_audioInit = true;
    } else {
      LOG_ERR("[VideoPlayer] MiniAudio init failed");
      m_hasAudio = false;
    }
  }

  LOG_INFO("[VideoPlayer] Initialized. Video=%s Audio=%s",
           m_videoCtx ? "Yes" : "No", m_hasAudio ? "Yes" : "No");

  m_duration = (double)m_fmtCtx->duration / AV_TIME_BASE;
  if (m_duration <= 0.0 && m_fmtCtx->bit_rate > 0) {
    m_duration = (double)(m_fileSize * 8) / m_fmtCtx->bit_rate;
  }

  // 6. Start Threads
  m_state = PlayState::Playing;
  m_demuxThread = std::thread([this]() { DemuxThread(); });
  m_vidDecThread = std::thread([this]() { VideoDecodeThread(); });
  m_audDecThread = std::thread([this]() { AudioDecodeThread(); });

  if (m_audioInit) {
    ma_device_start(&m_audioDevice);
  }

  // Fallback timer
  m_videoOnlyStartTime = ImGui::GetTime();

  return true;
}

void VideoPlayer::Close() {
  m_stopThreads = true;

  m_videoPktQueue.push(nullptr); // Wake up workers
  m_audioPktQueue.push(nullptr);
  m_frameQueue.push({}); // Empty frame

  if (m_demuxThread.joinable())
    m_demuxThread.join();
  if (m_vidDecThread.joinable())
    m_vidDecThread.join();
  if (m_audDecThread.joinable())
    m_audDecThread.join();

  if (m_audioInit) {
    ma_device_uninit(&m_audioDevice);
    m_audioInit = false;
  }

  if (m_fmtCtx)
    avformat_close_input(&m_fmtCtx);
  if (m_avioCtx) {
    av_freep(&m_avioCtx->buffer);
    avio_context_free(&m_avioCtx);
  }
  if (m_videoCtx)
    avcodec_free_context(&m_videoCtx);
  if (m_audioCtx)
    avcodec_free_context(&m_audioCtx);
  if (m_swsCtx)
    sws_freeContext(m_swsCtx);
  if (m_swrCtx)
    swr_free(&m_swrCtx);

  if (m_pbo[0])
    glDeleteBuffers(2, m_pbo);
  if (m_glTexture)
    glDeleteTextures(1, &m_glTexture);

  m_isOpen = false;
}

// ─── Playback Control ───────────────────────────────────────────────────────

void VideoPlayer::Play() {
  if (m_state == PlayState::Playing)
    return;
  m_state = PlayState::Playing;
  if (m_audioInit)
    ma_device_start(&m_audioDevice);
  if (!m_hasAudio) {
    // Resume video-only clock
    m_videoOnlyStartTime = ImGui::GetTime() - m_videoOnlyClock;
  }
}

void VideoPlayer::Pause() {
  if (m_state == PlayState::Paused)
    return;
  m_state = PlayState::Paused;
  if (m_audioInit)
    ma_device_stop(&m_audioDevice);
}

void VideoPlayer::Stop() {
  Pause();
  m_state = PlayState::Stopped;
  SeekByte(0);
}

void VideoPlayer::SeekByte(int64_t targetByte) {
  targetByte = std::clamp(targetByte, (int64_t)0, m_fileSize);
  m_seekTargetByte = targetByte;
  m_seekTimeOffset =
      m_duration > 0.0 ? ((double)targetByte / m_fileSize) * m_duration : 0.0;
  m_seekReq = true;
  m_forceFrameUpdate = true;

  // Clear queues to unblock threads quickly
  m_videoPktQueue.clear();
  m_audioPktQueue.clear();
  m_frameQueue.clear();
  m_audioRingBuf.clear();
}

// ─── Worker Threads ─────────────────────────────────────────────────────────

void VideoPlayer::DemuxThread() {
  while (!m_stopThreads) {
    if (m_seekReq) {
      int64_t pos = m_seekTargetByte.load();
      av_seek_frame(m_fmtCtx, -1, pos, AVSEEK_FLAG_BYTE);

      // Flush codecs
      if (m_videoCtx)
        avcodec_flush_buffers(m_videoCtx);
      if (m_audioCtx)
        avcodec_flush_buffers(m_audioCtx);

      // Reset clocks
      m_audioClock = 0.0;
      m_videoOnlyClock = 0.0;
      m_videoOnlyStartTime = ImGui::GetTime();

      m_seekReq = false;
      continue;
    }

    if (m_state == PlayState::Stopped) {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      continue;
    }

    // Throttle demuxing if queues are full
    if (m_videoPktQueue.size() >= kMaxVideoPkts ||
        m_audioPktQueue.size() >= kMaxAudioPkts) {
      std::this_thread::sleep_for(std::chrono::milliseconds(5));
      continue;
    }

    AVPacket *pkt = av_packet_alloc();
    int ret = av_read_frame(m_fmtCtx, pkt);
    if (ret >= 0) {
      // Track current byte position approx
      m_currentBytePos = avio_tell(m_fmtCtx->pb);

      if (pkt->stream_index == m_videoStream) {
        m_videoPktQueue.push(pkt);
      } else if (pkt->stream_index == m_audioStream) {
        m_audioPktQueue.push(pkt);
      } else {
        av_packet_free(&pkt);
      }
    } else {
      // EOF or Error
      av_packet_free(&pkt);
      if (ret == AVERROR_EOF) {
        // If it loops or stops, we sleep wait.
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
      } else {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
      }
    }
  }
}

void VideoPlayer::VideoDecodeThread() {
  AVFrame *frame = av_frame_alloc();
  double currentPtsBias = 0.0;
  bool firstFrameSinceSeek = true;

  while (!m_stopThreads) {
    if (m_seekReq) {
      firstFrameSinceSeek = true;
      std::this_thread::sleep_for(std::chrono::milliseconds(5));
      continue;
    }

    AVPacket *pkt = nullptr;
    if (!m_videoPktQueue.wait_and_pop(pkt, m_stopThreads)) {
      continue;
    }

    if (!pkt)
      break; // Wakeup pill

    if (avcodec_send_packet(m_videoCtx, pkt) == 0) {
      while (avcodec_receive_frame(m_videoCtx, frame) == 0) {

        // Throttle decoded frames
        while (m_frameQueue.size() >= kMaxVideoFrames && !m_stopThreads &&
               !m_seekReq) {
          std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }

        if (m_seekReq || m_stopThreads)
          break;

        // Handle PTS bias (PSS files have arbitrary non-zero PTS start)
        double rawPts = (frame->best_effort_timestamp != AV_NOPTS_VALUE)
                            ? frame->best_effort_timestamp * m_videoTimeBase
                            : 0.0;

        if (firstFrameSinceSeek) {
          currentPtsBias = rawPts;
          firstFrameSinceSeek = false;
          // Force the Audio clock to start from 0 for this segment
          m_audioClock = 0.0;
        }

        double normalizedPts = rawPts - currentPtsBias;
        if (normalizedPts < 0.0)
          normalizedPts = 0.0;

        int w = frame->width;
        int h = frame->height;

        m_swsCtx = sws_getCachedContext(
            m_swsCtx, w, h, (AVPixelFormat)frame->format, w, h, AV_PIX_FMT_RGBA,
            SWS_POINT, nullptr, nullptr, nullptr);

        FrameData fd;
        fd.pts = normalizedPts;
        fd.width = w;
        fd.height = h;
        fd.pixels.resize(w * h * 4);

        uint8_t *dst[1] = {fd.pixels.data()};
        int stride[1] = {w * 4};
        sws_scale(m_swsCtx, frame->data, frame->linesize, 0, h, dst, stride);

        m_frameQueue.push(std::move(fd));
      }
    }
    av_packet_free(&pkt);
  }
  av_frame_free(&frame);
}

void VideoPlayer::AudioDecodeThread() {
  AVFrame *frame = av_frame_alloc();
  std::vector<uint8_t> resampleBuf;

  while (!m_stopThreads) {
    if (m_seekReq || !m_hasAudio) {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      continue;
    }

    AVPacket *pkt = nullptr;
    if (!m_audioPktQueue.wait_and_pop(pkt, m_stopThreads)) {
      continue;
    }

    if (!pkt)
      break;

    if (avcodec_send_packet(m_audioCtx, pkt) == 0) {
      while (avcodec_receive_frame(m_audioCtx, frame) == 0) {

        while (m_audioRingBuf.size() >= kMaxAudioBufSize && !m_stopThreads &&
               !m_seekReq) {
          std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }

        if (m_seekReq || m_stopThreads)
          break;

        int outSamples = swr_get_out_samples(m_swrCtx, frame->nb_samples);
        size_t outSize = outSamples * m_audioChannels * sizeof(float);
        if (resampleBuf.size() < outSize)
          resampleBuf.resize(outSize);

        uint8_t *outData = resampleBuf.data();
        int got = swr_convert(m_swrCtx, &outData, outSamples,
                              (const uint8_t **)frame->data, frame->nb_samples);
        if (got > 0) {
          m_audioRingBuf.push(resampleBuf.data(),
                              got * m_audioChannels * sizeof(float));
        }
      }
    }
    av_packet_free(&pkt);
  }
  av_frame_free(&frame);
}

// ─── Audio Device Callback (Master Clock) ───────────────────────────────────

void VideoPlayer::AudioCallback(ma_device *pDevice, void *pOutput,
                                const void *pInput, ma_uint32 frameCount) {
  auto *player = static_cast<VideoPlayer *>(pDevice->pUserData);
  size_t reqBytes = frameCount * player->m_audioChannels * sizeof(float);

  size_t gotBytes =
      player->m_audioRingBuf.pop(static_cast<uint8_t *>(pOutput), reqBytes);

  // Silence missing samples
  if (gotBytes < reqBytes) {
    std::memset(static_cast<uint8_t *>(pOutput) + gotBytes, 0,
                reqBytes - gotBytes);
  }

  // Apply volume
  if (player->m_muted || player->m_volume != 1.0f) {
    float *outFloat = static_cast<float *>(pOutput);
    float vol = player->m_muted ? 0.0f : player->m_volume;
    for (ma_uint32 i = 0; i < frameCount * player->m_audioChannels; i++) {
      outFloat[i] *= vol;
    }
  }

  // Update master clock based on actual played hardware samples
  if (gotBytes > 0) {
    double clockIncrement =
        (double)(gotBytes / (player->m_audioChannels * sizeof(float))) /
        (double)player->m_audioSampleRate;
    double current = player->m_audioClock.load(std::memory_order_relaxed);
    player->m_audioClock.store(current + clockIncrement,
                               std::memory_order_relaxed);
    player->m_lastAudioCallbackTime.store(ImGui::GetTime(),
                                          std::memory_order_relaxed);
    player->m_isStarving.store(false, std::memory_order_relaxed);
  } else {
    player->m_isStarving.store(true, std::memory_order_relaxed);
  }
}

// ─── UI & Presentation ──────────────────────────────────────────────────────

void VideoPlayer::UploadFrame(const FrameData &fd) {
  if (m_texW != fd.width || m_texH != fd.height || !m_glTexture) {
    if (m_pbo[0]) {
      glDeleteBuffers(2, m_pbo);
      m_pbo[0] = m_pbo[1] = 0;
    }
    if (m_glTexture)
      glDeleteTextures(1, &m_glTexture);

    m_texW = fd.width;
    m_texH = fd.height;

    glGenTextures(1, &m_glTexture);
    glBindTexture(GL_TEXTURE_2D, m_glTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_texW, m_texH, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, nullptr);
  }

  glBindTexture(GL_TEXTURE_2D, m_glTexture);
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_texW, m_texH, GL_RGBA,
                  GL_UNSIGNED_BYTE, fd.pixels.data());
}

void VideoPlayer::DrawVideo() {
  double now = ImGui::GetTime();

  // Update Fallback Clock
  if (m_state == PlayState::Playing && !m_hasAudio) {
    m_videoOnlyClock = now - m_videoOnlyStartTime;
  }

  // Interpolate Master Clock for microsecond smoothness instead of ~11ms audio
  // chunks
  double masterClock = 0.0;
  if (m_hasAudio) {
    masterClock = m_audioClock.load(std::memory_order_relaxed);
    if (!m_isStarving.load(std::memory_order_relaxed) &&
        m_state == PlayState::Playing) {
      double timeSinceLast =
          now - m_lastAudioCallbackTime.load(std::memory_order_relaxed);
      // Protect against clock runaway if callback dies
      if (timeSinceLast >= 0.0 && timeSinceLast < 0.1) {
        masterClock += timeSinceLast;
      }
    }
  } else {
    masterClock = m_videoOnlyClock;
  }
  m_currentTime = masterClock + m_seekTimeOffset;

  // A/V Sync Logic
  if (m_state == PlayState::Playing) {
    bool hasDueFrame = false;
    FrameData latestDueFrame;

    while (m_frameQueue.size() > 0) {
      double pts = m_frameQueue.front_pts();
      if (pts < 0.0)
        break;

      // If the frame is from the future, stop popping and wait for next ImGui
      // cycle
      if (pts > masterClock) {
        break;
      }

      // Frame is due or late. Pop it.
      m_frameQueue.pop(latestDueFrame);
      hasDueFrame = true;
    }

    if (hasDueFrame) {
      UploadFrame(latestDueFrame);
    }
  } else {
    // Paused/Stopped - just grab one frame if a force-update was requested
    // (e.g. from seek scrubbing)
    if (m_forceFrameUpdate && m_frameQueue.size() > 0) {
      FrameData fd;
      if (m_frameQueue.pop(fd)) {
        UploadFrame(fd);
        m_forceFrameUpdate = false;
      }
    }
  }

  // Draw ImGui Texture
  const ImVec2 avail = ImGui::GetContentRegionAvail();
  if (avail.x > 0 && avail.y > 0 && m_glTexture) {
    const float aspect = (float)m_texH / (float)m_texW;
    float drawW = avail.x, drawH = drawW * aspect;
    if (drawH > avail.y) {
      drawH = avail.y;
      drawW = drawH / aspect;
    }

    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (avail.x - drawW) * 0.5f);
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (avail.y - drawH) * 0.5f);
    ImGui::Image((void *)(intptr_t)m_glTexture, ImVec2(drawW, drawH));

    if (ImGui::IsItemHovered() &&
        ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
      if (m_state == PlayState::Playing)
        Pause();
      else
        Play();
    }
  } else if (!m_glTexture) {
    const char *msg = "Buffering...";
    ImVec2 ts = ImGui::CalcTextSize(msg);
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (avail.x - ts.x) * 0.5f);
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (avail.y - ts.y) * 0.5f);
    ImGui::TextDisabled("%s", msg);
  }

  // Info overlay moved to global Inspector

  // Handle shortcuts if the window is focused/hovered
  if (ImGui::IsWindowFocused() || ImGui::IsWindowHovered()) {
    if (ImGui::IsKeyPressed(ImGuiKey_Space)) {
      if (m_state == PlayState::Playing)
        Pause();
      else
        Play();
    }
    if (ImGui::IsKeyPressed(ImGuiKey_RightArrow)) {
      int64_t step = m_duration > 0 ? (int64_t)((5.0 / m_duration) * m_fileSize)
                                    : (int64_t)(m_fileSize * 0.05);
      SeekByte(std::min(m_currentBytePos + step, m_fileSize));
    }
    if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow)) {
      int64_t step = m_duration > 0 ? (int64_t)((5.0 / m_duration) * m_fileSize)
                                    : (int64_t)(m_fileSize * 0.05);
      SeekByte(std::max(m_currentBytePos - step, (int64_t)0));
    }
    if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) {
      m_volume = std::min(m_volume + 0.05f, 1.5f);
    }
    if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) {
      m_volume = std::max(m_volume - 0.05f, 0.0f);
    }
  }
}

void VideoPlayer::DrawToolbar() {
  if (!m_isOpen)
    return;

  ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                        ImVec4(0.25f, 0.25f, 0.30f, 0.90f));

  if (m_state == PlayState::Playing) {
    if (ImGui::SmallButton(ICON_SF_PAUSE_FILL " Pause"))
      Pause();
  } else {
    if (ImGui::SmallButton(ICON_SF_PLAY_FILL " Play"))
      Play();
  }
  ImGui::SameLine();

  bool canStop = (m_state != PlayState::Stopped);
  if (!canStop)
    ImGui::BeginDisabled();
  if (ImGui::SmallButton(ICON_SF_STOP_FILL " Stop"))
    Stop();
  if (!canStop)
    ImGui::EndDisabled();

  ImGui::SameLine();
  ImGui::TextDisabled("|");
  ImGui::SameLine();

  // Time display
  auto formatTime = [](int totalSecs) {
    int h = totalSecs / 3600;
    int m = (totalSecs % 3600) / 60;
    int s = totalSecs % 60;
    char buf[32];
    if (h > 0)
      snprintf(buf, sizeof(buf), "%02d:%02d:%02d", h, m, s);
    else
      snprintf(buf, sizeof(buf), "%02d:%02d", m, s);
    return std::string(buf);
  };

  int curSec = (int)m_currentTime;

  if (m_duration > 0.0) {
    int durSec = (int)m_duration;
    ImGui::TextDisabled("%s / %s", formatTime(curSec).c_str(),
                        formatTime(durSec).c_str());
  } else {
    ImGui::TextDisabled("%s", formatTime(curSec).c_str());
  }

  ImGui::SameLine();

  // Info toggle was migrated natively to global Inspector
  ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 175.f);

  // Mute toggle
  if (ImGui::SmallButton(m_muted ? ICON_SF_SPEAKER_SLASH_FILL
                                 : ICON_SF_SPEAKER_WAVE_2_FILL))
    m_muted = !m_muted;
  if (ImGui::IsItemHovered())
    ImGui::SetTooltip("Mute");

  // Volume slider
  ImGui::SameLine();
  ImGui::SetNextItemWidth(80.0f);
  float volPercent = m_volume * 100.0f;
  if (ImGui::SliderFloat("##volume", &volPercent, 0.0f, 150.0f, "%.0f%%")) {
    m_volume = volPercent / 100.0f;
  }
  if (ImGui::IsItemHovered())
    ImGui::SetTooltip("Volume (Up/Down Arrow)");

  ImGui::PopStyleColor(2);
  ImGui::SameLine();
  ImGui::TextDisabled("|");
  ImGui::SameLine();

  // ─── Byte-Position Slider ───────────────────────────────────────────────
  // PSS files sometimes have unknown duration
  float filePct =
      m_fileSize > 0 ? (float)m_currentBytePos / (float)m_fileSize : 0.0f;
  if (m_duration > 0.0) {
    filePct = m_currentTime / m_duration;
  }

  if (m_sliderHeld)
    filePct = m_sliderPreviewPos;

  float rightW =
      ImGui::CalcTextSize("100%").x + ImGui::GetStyle().ItemSpacing.x * 2.0f;
  ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - rightW);

  if (ImGui::SliderFloat("##seek", &filePct, 0.0f, 1.0f, "")) {
    // Handled in IsItemActive
  }

  if (ImGui::IsItemActive()) {
    m_sliderHeld = true;
    m_sliderPreviewPos = filePct;

    // Throttled live-scrub via demux-seek
    double now = ImGui::GetTime();
    if (now - m_lastScrubTime > 0.1) { // 100ms debounce
      SeekByte((int64_t)(filePct * m_fileSize));
      m_lastScrubTime = now;
    }
  }
  if (ImGui::IsItemDeactivatedAfterEdit()) {
    m_sliderHeld = false;
    SeekByte((int64_t)(filePct * m_fileSize));
    if (m_state == PlayState::Stopped)
      Play();
  }

  ImGui::SameLine();
  ImGui::TextDisabled("%3.0f%%", filePct * 100.0f);
}

void VideoPlayer::DrawInspector(AppContext &ctx) {
  ImGui::TextWrapped("File: %s", m_name.c_str());
  ImGui::Separator();

  if (ImGui::CollapsingHeader("Video Stream", ImGuiTreeNodeFlags_DefaultOpen)) {
    ImGui::Text("Codec: %s", m_videoCodecName.c_str());
    ImGui::Text("Resolution: %dx%d", m_videoWidth, m_videoHeight);
    ImGui::Text("File FPS: %.2f FPS", m_fps);
    ImGui::Text("App Render: %.1f FPS", ImGui::GetIO().Framerate);
    ImGui::Text("Time Base: %.5f", m_videoTimeBase);
  }

  if (ImGui::CollapsingHeader("Audio Stream", ImGuiTreeNodeFlags_DefaultOpen)) {
    if (m_hasAudio) {
      ImGui::Text("Codec: %s", m_audioCodecName.c_str());
      ImGui::Text("Sample Rate: %u Hz", m_audioSampleRate);
      ImGui::Text("Channels: %u", m_audioChannels);
      ImGui::Text("Time Base: %.5f", m_audioTimeBase);
    } else {
      ImGui::TextDisabled("No audio stream.");
    }
  }

  if (ImGui::CollapsingHeader("Playback Engine",
                              ImGuiTreeNodeFlags_DefaultOpen)) {
    ImGui::Text("Hardware: OpenGL / SWS_POINT");
    ImGui::Text("File Size: %lld bytes", m_fileSize);
    ImGui::Text("Duration: %.2f sec", m_duration);
    ImGui::Text("A/V Desync: %.3f s",
                m_hasAudio ? m_audioClock.load() - m_currentTime : 0.0);
  }
}

void VideoPlayer::Draw() {
  DrawToolbar();
  ImGui::Separator();
  DrawVideo();
}

} // namespace GOW