#pragma once

// TaskManager — Centralized async task execution with progress tracking.
// Adapted from ImHex's TaskManager (WerWolv, LGPLv2.1)
// Simplified: std::thread pool + atomic stop flag, no localization, no
// source_location. std::jthread was avoided because Apple Clang's libc++
// in the macOS-14 CI image still lacks <stop_token>.

#include <atomic>
#include <condition_variable>
#include <functional>
#include <list>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include <thread>
#include <cstdint>

namespace GOW {

    /// Exception thrown when a task is interrupted. Worker threads catch this.
    struct TaskInterruptor {};

    /// Represents a single async task with progress tracking and cancellation.
    class Task {
    public:
        Task(std::string name, uint64_t maxValue, bool background,
             std::function<void(Task&)> function);
        Task(Task&& other) noexcept;
        ~Task();

        // ── Progress ──
        /// Update current progress value. Checks for interruption.
        void update(uint64_t value);
        /// Check for interruption without updating progress.
        void update() const;
        /// Increment progress by 1. Checks for interruption.
        void increment();
        /// Set the maximum value for progress calculation.
        void setMaxValue(uint64_t value);

        // ── State queries ──
        bool isFinished() const;
        bool isBackgroundTask() const;
        bool hadException() const;
        bool shouldInterrupt() const;
        bool wasInterrupted() const;

        const std::string& getName() const { return m_name; }
        uint64_t getValue() const;
        uint64_t getMaxValue() const;
        std::string getExceptionMessage() const;

        // ── Control ──
        void interrupt();
        void setInterruptCallback(std::function<void()> callback);
        void wait() const;

    private:
        friend class TaskManager;

        void finish();
        void interruption();
        void exception(const char* message);
        void clearException();

        std::string                  m_name;
        std::function<void(Task&)>   m_function;
        std::atomic<uint64_t>        m_maxValue{0};
        std::atomic<uint64_t>        m_currValue{0};
        std::atomic_flag             m_finished{};
        std::atomic_flag             m_hadException{};
        std::atomic_flag             m_interrupted{};
        std::atomic<bool>            m_shouldInterrupt{false};
        bool                         m_background = false;
        std::string                  m_exceptionMessage;
        mutable std::mutex           m_mutex;
        std::function<void()>        m_interruptCallback;
    };


    /// Non-owning handle to a Task for external progress monitoring.
    class TaskHolder {
    public:
        TaskHolder() = default;
        explicit TaskHolder(std::weak_ptr<Task> task) : m_task(std::move(task)) {}

        bool isRunning() const;
        bool hadException() const;
        bool shouldInterrupt() const;
        bool wasInterrupted() const;

        void interrupt() const;
        void wait() const;

        /// Returns progress percentage (0–100), or 0 if indeterminate.
        uint32_t getProgress() const;

    private:
        std::weak_ptr<Task> m_task;
    };


    /// Central task manager — thread pool with job queue.
    class TaskManager {
    public:
        /// Initialize the thread pool (call once at startup).
        static void init();

        /// Shutdown the thread pool (call once at exit).
        static void exit();

        // ── Task creation ──

        /// Create a foreground task (visible in StatusBar with progress).
        static TaskHolder createTask(const std::string& name, uint64_t maxValue,
                                     std::function<void(Task&)> function);

        /// Create a foreground task without Task& reference (simple lambda).
        static TaskHolder createTask(const std::string& name, uint64_t maxValue,
                                     std::function<void()> function);

        /// Create a background task (invisible in StatusBar, no progress).
        static TaskHolder createBackgroundTask(const std::string& name,
                                               std::function<void(Task&)> function);

        /// Create a background task (simple lambda).
        static TaskHolder createBackgroundTask(const std::string& name,
                                               std::function<void()> function);

        // ── Deferred calls (run on main thread) ──

        /// Queue a function to run on the main thread at the start of next frame.
        static void doLater(const std::function<void()>& function);

        /// Run all queued deferred calls. Call this in the main frame loop.
        static void runDeferredCalls();

        // ── Status queries ──

        /// Get list of all running tasks (for StatusBar rendering).
        static const std::list<std::shared_ptr<Task>>& getRunningTasks();

        /// Count of visible (non-background) running tasks.
        static size_t getRunningTaskCount();

        /// Count of background running tasks.
        static size_t getRunningBackgroundTaskCount();

        /// Remove finished tasks from the list.
        static void collectGarbage();

        /// Callback invoked when all tasks finish.
        static void runWhenTasksFinished(const std::function<void()>& function);

    private:
        static TaskHolder createTask(const std::string& name, uint64_t maxValue,
                                     bool background,
                                     std::function<void(Task&)> function);
    };

} // namespace GOW
