#include "Core/TaskManager.h"
#include "Core/Logger.h"

#include <algorithm>
#include <ranges>

namespace Onyx {

    // â”€â”€ Static storage â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

    namespace {
        std::recursive_mutex s_deferredCallsMutex;
        std::mutex           s_tasksFinishedMutex;

        std::list<std::shared_ptr<Task>>     s_tasks;
        std::list<std::shared_ptr<Task>>     s_taskQueue;
        std::list<std::function<void()>>     s_deferredCalls;
        std::list<std::function<void()>>     s_tasksFinishedCallbacks;

        std::mutex                           s_queueMutex;
        std::condition_variable              s_jobCondVar;
        std::vector<std::thread>             s_workers;
        std::atomic<bool>                    s_shouldStop{false};
    }

    // â”€â”€ Task â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

    Task::Task(std::string name, uint64_t maxValue, bool background,
               std::function<void(Task&)> function)
        : m_name(std::move(name))
        , m_function(std::move(function))
        , m_maxValue(maxValue)
        , m_background(background)
    {}

    Task::Task(Task&& other) noexcept {
        {
            std::scoped_lock thisLock(m_mutex);
            std::scoped_lock otherLock(other.m_mutex);
            m_function = std::move(other.m_function);
            m_name     = std::move(other.m_name);
        }

        m_maxValue.store(other.m_maxValue.load());
        m_currValue.store(other.m_currValue.load());

        if (other.m_finished.test())    m_finished.test_and_set();
        if (other.m_hadException.test()) m_hadException.test_and_set();
        if (other.m_interrupted.test()) m_interrupted.test_and_set();

        m_shouldInterrupt.store(other.m_shouldInterrupt.load());
    }

    Task::~Task() {
        if (!isFinished())
            interrupt();
    }

    void Task::update(uint64_t value) {
        m_currValue.store(value, std::memory_order_relaxed);
        if (m_shouldInterrupt.load(std::memory_order_relaxed)) [[unlikely]]
            throw TaskInterruptor();
    }

    void Task::update() const {
        if (m_shouldInterrupt.load(std::memory_order_relaxed)) [[unlikely]]
            throw TaskInterruptor();
    }

    void Task::increment() {
        m_currValue.fetch_add(1, std::memory_order_relaxed);
        if (m_shouldInterrupt.load(std::memory_order_relaxed)) [[unlikely]]
            throw TaskInterruptor();
    }

    void Task::setMaxValue(uint64_t value) { m_maxValue = value; }

    void Task::interrupt() {
        m_shouldInterrupt = true;
        if (m_interruptCallback)
            m_interruptCallback();
    }

    void Task::setInterruptCallback(std::function<void()> callback) {
        m_interruptCallback = std::move(callback);
    }

    bool Task::isBackgroundTask() const { return m_background; }
    bool Task::isFinished() const      { return m_finished.test(); }
    bool Task::hadException() const    { return m_hadException.test(); }
    bool Task::shouldInterrupt() const { return m_shouldInterrupt; }
    bool Task::wasInterrupted() const  { return m_interrupted.test(); }

    void Task::clearException()  { m_hadException.clear(); }

    std::string Task::getExceptionMessage() const {
        std::scoped_lock lock(m_mutex);
        return m_exceptionMessage;
    }

    uint64_t Task::getValue() const    { return m_currValue; }
    uint64_t Task::getMaxValue() const { return m_maxValue; }

    void Task::wait() const  { m_finished.wait(false); }
    void Task::finish()      { m_finished.test_and_set(); m_finished.notify_all(); }
    void Task::interruption(){ m_interrupted.test_and_set(); m_interrupted.notify_all(); }

    void Task::exception(const char* message) {
        std::scoped_lock lock(m_mutex);
        m_exceptionMessage = message;
        m_hadException.test_and_set();
        m_hadException.notify_all();
        if (m_interruptCallback)
            m_interruptCallback();
    }

    // â”€â”€ TaskHolder â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

    bool TaskHolder::isRunning() const {
        if (auto t = m_task.lock()) return !t->isFinished();
        return false;
    }

    bool TaskHolder::hadException() const {
        if (auto t = m_task.lock()) return t->hadException();
        return false;
    }

    bool TaskHolder::shouldInterrupt() const {
        if (auto t = m_task.lock()) return t->shouldInterrupt();
        return false;
    }

    bool TaskHolder::wasInterrupted() const {
        if (auto t = m_task.lock()) return t->wasInterrupted();
        return false;
    }

    void TaskHolder::interrupt() const {
        if (auto t = m_task.lock()) t->interrupt();
    }

    void TaskHolder::wait() const {
        if (auto t = m_task.lock()) t->wait();
    }

    uint32_t TaskHolder::getProgress() const {
        if (auto t = m_task.lock()) {
            if (t->getMaxValue() == 0) return 0;
            return static_cast<uint32_t>((t->getValue() * 100) / t->getMaxValue());
        }
        return 0;
    }

    // â”€â”€ TaskManager â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

    void TaskManager::init() {
        const auto threadCount = std::max(2u, std::thread::hardware_concurrency());

        LOG_INFO("[TaskManager] Initializing thread pool with %u workers", threadCount);

        s_shouldStop.store(false);

        for (uint32_t i = 0; i < threadCount; i++) {
            s_workers.emplace_back([] {
                while (true) {
                    std::shared_ptr<Task> task;

                    {
                        std::unique_lock lock(s_queueMutex);
                        s_jobCondVar.wait(lock, [] {
                            return !s_taskQueue.empty() || s_shouldStop.load();
                        });

                        if (s_shouldStop.load())
                            break;

                        task = std::move(s_taskQueue.front());
                        s_taskQueue.pop_front();
                    }

                    try {
                        task->m_function(*task);
                        LOG_DEBUG("[TaskManager] Task '%s' finished", task->getName().c_str());
                    } catch (const TaskInterruptor&) {
                        task->interruption();
                    } catch (const std::exception& e) {
                        LOG_ERR("[TaskManager] Exception in task '%s': %s",
                                task->getName().c_str(), e.what());
                        task->exception(e.what());
                    } catch (...) {
                        LOG_ERR("[TaskManager] Unknown exception in task '%s'",
                                task->getName().c_str());
                        task->exception("Unknown Exception");
                    }

                    task->finish();
                }
            });
        }
    }

    void TaskManager::exit() {
        // Interrupt all tasks
        for (const auto& task : s_tasks)
            task->interrupt();

        // Ask worker threads to exit and wake them up
        {
            std::unique_lock lock(s_queueMutex);
            s_shouldStop.store(true);
            s_jobCondVar.notify_all();
        }

        // Join all workers
        for (auto& thread : s_workers) {
            if (thread.joinable())
                thread.join();
        }

        s_workers.clear();
        s_tasks.clear();
        s_taskQueue.clear();
        s_deferredCalls.clear();
        s_tasksFinishedCallbacks.clear();
    }

    TaskHolder TaskManager::createTask(const std::string& name, uint64_t maxValue,
                                       bool background,
                                       std::function<void(Task&)> function) {
        std::scoped_lock lock(s_queueMutex);

        auto task = std::make_shared<Task>(name, maxValue, background, std::move(function));
        s_tasks.emplace_back(task);
        s_taskQueue.emplace_back(std::move(task));
        s_jobCondVar.notify_one();

        return TaskHolder(s_tasks.back());
    }

    TaskHolder TaskManager::createTask(const std::string& name, uint64_t maxValue,
                                       std::function<void(Task&)> function) {
        LOG_INFO("[TaskManager] Creating task '%s'", name.c_str());
        return createTask(name, maxValue, false, std::move(function));
    }

    TaskHolder TaskManager::createTask(const std::string& name, uint64_t maxValue,
                                       std::function<void()> function) {
        LOG_INFO("[TaskManager] Creating task '%s'", name.c_str());
        return createTask(name, maxValue, false,
            [function = std::move(function)](Task&) { function(); });
    }

    TaskHolder TaskManager::createBackgroundTask(const std::string& name,
                                                  std::function<void(Task&)> function) {
        LOG_DEBUG("[TaskManager] Creating background task '%s'", name.c_str());
        return createTask(name, 0, true, std::move(function));
    }

    TaskHolder TaskManager::createBackgroundTask(const std::string& name,
                                                  std::function<void()> function) {
        LOG_DEBUG("[TaskManager] Creating background task '%s'", name.c_str());
        return createTask(name, 0, true,
            [function = std::move(function)](Task&) { function(); });
    }

    void TaskManager::collectGarbage() {
        {
            std::scoped_lock lock(s_queueMutex);
            std::erase_if(s_tasks, [](const auto& task) {
                return task->isFinished() && !task->hadException();
            });
        }

        if (s_tasks.empty()) {
            std::scoped_lock lock(s_tasksFinishedMutex);
            for (const auto& call : s_tasksFinishedCallbacks)
                call();
            s_tasksFinishedCallbacks.clear();
        }
    }

    const std::list<std::shared_ptr<Task>>& TaskManager::getRunningTasks() {
        return s_tasks;
    }

    size_t TaskManager::getRunningTaskCount() {
        std::scoped_lock lock(s_queueMutex);
        return std::ranges::count_if(s_tasks, [](const auto& task) {
            return !task->isBackgroundTask();
        });
    }

    size_t TaskManager::getRunningBackgroundTaskCount() {
        std::scoped_lock lock(s_queueMutex);
        return std::ranges::count_if(s_tasks, [](const auto& task) {
            return task->isBackgroundTask();
        });
    }

    void TaskManager::doLater(const std::function<void()>& function) {
        std::scoped_lock lock(s_deferredCallsMutex);
        s_deferredCalls.push_back(function);
    }

    void TaskManager::runDeferredCalls() {
        std::scoped_lock lock(s_deferredCallsMutex);
        while (!s_deferredCalls.empty()) {
            auto callback = s_deferredCalls.front();
            s_deferredCalls.pop_front();
            callback();
        }
    }

    void TaskManager::runWhenTasksFinished(const std::function<void()>& function) {
        std::scoped_lock lock(s_tasksFinishedMutex);
        for (const auto& task : s_tasks)
            task->interrupt();
        s_tasksFinishedCallbacks.push_back(function);
    }

} // namespace Onyx
