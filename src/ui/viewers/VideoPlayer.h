#pragma once

#include "ui/viewers/IDocumentContent.h"
#include "core/vfs/IFile.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}

#include <miniaudio.h>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <vector>
#include <string>
#include <memory>

namespace GOW {

// ─── Thread-Safe Queue ──────────────────────────────────────────────────────
template <typename T>
class ThreadSafeQueue {
public:
    void push(T item) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.push(std::move(item));
        m_cond.notify_one();
    }

    bool pop(T& outItem) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_queue.empty()) return false;
        outItem = std::move(m_queue.front());
        m_queue.pop();
        return true;
    }

    // Blocking pop with a stop flag
    bool wait_and_pop(T& outItem, const std::atomic<bool>& stopFlag) {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cond.wait(lock, [this, &stopFlag] { return !m_queue.empty() || stopFlag.load(); });
        if (m_queue.empty()) return false;
        outItem = std::move(m_queue.front());
        m_queue.pop();
        return true;
    }

    void clear() {
        std::lock_guard<std::mutex> lock(m_mutex);
        while (!m_queue.empty()) m_queue.pop();
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_queue.size();
    }

    bool empty() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_queue.empty();
    }

    // Peeks the PTS of the front element (assuming T is FrameData)
    double front_pts() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_queue.empty()) return -1.0;
        return m_queue.front().pts; // works for FrameData
    }

private:
    std::queue<T> m_queue;
    mutable std::mutex m_mutex;
    std::condition_variable m_cond;
};

// ─── Custom structs for queue items ──────────────────────────────────────────
struct PacketData {
    AVPacket* pkt = nullptr; // We'll manage allocation manually or via wrappers
};

struct FrameData {
    double pts = 0.0;
    int width = 0;
    int height = 0;
    std::vector<uint8_t> pixels; // RGBA pixels
};

// ─── Simple Audio Ring Buffer ────────────────────────────────────────────────
class AudioRingBuffer {
public:
    AudioRingBuffer(size_t capacity = 1024 * 1024) : m_buffer(capacity), m_capacity(capacity) {}
    void push(const uint8_t* data, size_t size);
    size_t pop(uint8_t* outData, size_t reqSize);
    void clear();
    size_t size() const;
private:
    std::vector<uint8_t> m_buffer;
    size_t m_capacity;
    size_t m_writePos = 0;
    size_t m_readPos = 0;
    size_t m_size = 0;
    mutable std::mutex m_mutex;
};

// ─── Playback State ─────────────────────────────────────────────────────────
enum class PlayState {
    Playing,
    Paused,
    Stopped
};

// ─── VideoPlayer 2.0 ────────────────────────────────────────────────────────
class VideoPlayer : public IDocumentContent {
public:
    VideoPlayer(const std::string& name, std::shared_ptr<IFile> file);
    ~VideoPlayer() override;

    std::string GetName() const override { return m_name; }
    void Draw() override;

private:
    bool FinishOpen();
    void Close();

    // Transport
    void Play();
    void Pause();
    void Stop();
    void SeekByte(int64_t targetByte);

    // Threads
    void DemuxThread();
    void VideoDecodeThread();
    void AudioDecodeThread();

    // UI
    void DrawToolbar();
    void DrawVideo();
    void DrawInspector(AppContext& ctx) override;
    void UploadFrame(const FrameData& fd);

    static void AudioCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount);

private:
    std::string m_name;
    std::shared_ptr<IFile> m_ifile;
    bool m_isOpen = false;

    // FFmpeg Contexts
    AVIOContext* m_avioCtx = nullptr;
    AVFormatContext* m_fmtCtx = nullptr;
    AVCodecContext* m_videoCtx = nullptr;
    AVCodecContext* m_audioCtx = nullptr;
    SwsContext* m_swsCtx = nullptr;
    SwrContext* m_swrCtx = nullptr;

    int m_videoStream = -1;
    int m_audioStream = -1;
    double m_videoTimeBase = 0.0;
    double m_audioTimeBase = 0.0;
    int64_t m_fileSize = 0;

    // Multithreading & Queues
    std::atomic<bool> m_stopThreads{false};
    std::thread m_demuxThread;
    std::thread m_vidDecThread;
    std::thread m_audDecThread;

    ThreadSafeQueue<AVPacket*> m_videoPktQueue;
    ThreadSafeQueue<AVPacket*> m_audioPktQueue;
    ThreadSafeQueue<FrameData> m_frameQueue;
    AudioRingBuffer m_audioRingBuf;

    // Constants for queue caps
    const size_t kMaxVideoPkts = 200;
    const size_t kMaxAudioPkts = 500;
    const size_t kMaxVideoFrames = 15;
    const size_t kMaxAudioBufSize = 1024 * 1024; // 1MB

    // Synchronization & State
    std::atomic<PlayState> m_state{PlayState::Playing};
    std::atomic<bool> m_seekReq{false};
    std::atomic<int64_t> m_seekTargetByte{0};
    
    // The Master Clock (driven by audio playback)
    std::atomic<double> m_audioClock{0.0};
    std::atomic<double> m_lastAudioCallbackTime{0.0};
    std::atomic<bool> m_isStarving{true};
    
    // Fallback clock for when there's no audio stream
    double m_videoOnlyStartTime = 0.0;
    double m_videoOnlyClock = 0.0;
    std::atomic<bool> m_hasAudio{false};

    // Audio Output
    ma_device m_audioDevice;
    bool m_audioInit = false;
    uint32_t m_audioChannels = 2;
    uint32_t m_audioSampleRate = 44100;

    // UI & Rendering State
    unsigned int m_glTexture = 0;
    unsigned int m_pbo[2] = {0, 0};
    int m_pboWrite = 0;
    int m_texW = 0, m_texH = 0;

    float m_sliderPreviewPos = 0.0f;
    bool m_sliderHeld = false;
    double m_lastScrubTime = 0.0;
    int64_t m_currentBytePos = 0; // for slider position

    // Perf tracking
    double m_frameDuration = 1.0 / 30.0;
    double m_lastRenderTime = 0.0;

    // Volume & Time
    float m_volume = 1.0f;
    bool m_muted = false;
    double m_duration = 0.0; // In seconds
    double m_currentTime = 0.0; // In seconds
    double m_seekTimeOffset = 0.0;

    // Stream Info
    std::string m_videoCodecName = "Unknown";
    std::string m_audioCodecName = "Unknown";
    int m_videoWidth = 0;
    int m_videoHeight = 0;
    double m_fps = 0.0;
    bool m_forceFrameUpdate = false;
};

} // namespace GOW