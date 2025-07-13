/**
 * @file ThreadPool.cpp
 * @brief Thread pool implementation for concurrent task execution
 */

#include "als/core/ThreadPool.h"
#include <iostream>
#include <algorithm>

namespace als {
namespace core {

ThreadPool::ThreadPool(size_t num_threads, size_t max_queue_size)
    : stop_(false)
    , active_threads_(0)
    , max_queue_size_(max_queue_size) {

    if (num_threads == 0) {
        num_threads = std::thread::hardware_concurrency();
        if (num_threads == 0) {
            num_threads = 4; // Fallback
        }
    }

    // Clamp to reasonable limits
    num_threads = std::min(num_threads, size_t(16));

    std::cout << "[ThreadPool] Creating thread pool with " << num_threads << " threads" << std::endl;

    workers_.reserve(num_threads);
    for (size_t i = 0; i < num_threads; ++i) {
        workers_.emplace_back([this] { workerLoop(); });
    }
}

ThreadPool::~ThreadPool() {
    std::cout << "[ThreadPool] Shutting down thread pool" << std::endl;

    {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        stop_ = true;
    }

    condition_.notify_all();

    for (std::thread& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }

    std::cout << "[ThreadPool] Thread pool shutdown complete" << std::endl;
}

std::shared_ptr<std::atomic<bool>> ThreadPool::createCancellationToken() {
    return std::make_shared<std::atomic<bool>>(false);
}

void ThreadPool::cancelAllTasks() {
    std::unique_lock<std::mutex> lock(queue_mutex_);

    // Clear the queue and mark tasks as cancelled
    size_t cancelled_count = task_queue_.size();
    while (!task_queue_.empty()) {
        task_queue_.pop();
    }

    {
        std::lock_guard<std::mutex> stats_lock(stats_mutex_);
        stats_.cancelled += cancelled_count;
    }

    std::cout << "[ThreadPool] Cancelled " << cancelled_count << " pending tasks" << std::endl;
}

bool ThreadPool::waitForCompletion(std::chrono::milliseconds timeout) {
    auto start_time = std::chrono::steady_clock::now();

    while (true) {
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            if (task_queue_.empty() && active_threads_.load() == 0) {
                return true; // All tasks completed
            }
        }

        if (timeout != std::chrono::milliseconds::max()) {
            auto elapsed = std::chrono::steady_clock::now() - start_time;
            if (elapsed >= timeout) {
                return false; // Timeout
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void ThreadPool::resize(size_t num_threads) {
    if (num_threads == workers_.size()) {
        return;
    }

    std::cout << "[ThreadPool] Resizing from " << workers_.size()
              << " to " << num_threads << " threads" << std::endl;

    if (num_threads < workers_.size()) {
        // Shrink pool - this is complex, so let's use a simpler approach
        // Stop all threads and recreate the pool
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            stop_ = true;
        }
        condition_.notify_all();

        // Join all threads
        for (auto& worker : workers_) {
            if (worker.joinable()) {
                worker.join();
            }
        }

        // Clear and recreate with new size
        workers_.clear();
        stop_ = false;

        workers_.reserve(num_threads);
        for (size_t i = 0; i < num_threads; ++i) {
            workers_.emplace_back([this] { workerLoop(); });
        }
    } else {
        // Grow pool
        workers_.reserve(num_threads);
        for (size_t i = workers_.size(); i < num_threads; ++i) {
            workers_.emplace_back([this] { workerLoop(); });
        }
    }
}

size_t ThreadPool::queuedTasks() const {
    std::unique_lock<std::mutex> lock(queue_mutex_);
    return task_queue_.size();
}

ThreadPool::TaskStats ThreadPool::getStats() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return stats_;
}

void ThreadPool::resetStats() {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    stats_ = TaskStats{};
}

void ThreadPool::workerLoop() {
    while (true) {
        Task task;

        {
            std::unique_lock<std::mutex> lock(queue_mutex_);

            // Wait for a task or stop signal
            condition_.wait(lock, [this] {
                return stop_ || !task_queue_.empty();
            });

            if (stop_ && task_queue_.empty()) {
                break; // Exit worker thread
            }

            if (!task_queue_.empty()) {
                task = std::move(const_cast<Task&>(task_queue_.top()));
                task_queue_.pop();
            } else {
                continue; // Spurious wakeup
            }
        }

        // Execute the task
        active_threads_++;

        try {
            if (!shouldCancel(task.cancellation_token)) {
                task.function();
            }
        } catch (const std::exception& e) {
            std::cerr << "[ThreadPool] Task execution failed: " << e.what() << std::endl;
        } catch (...) {
            std::cerr << "[ThreadPool] Task execution failed with unknown exception" << std::endl;
        }

        active_threads_--;
    }
}

bool ThreadPool::shouldCancel(const std::shared_ptr<std::atomic<bool>>& token) const {
    return token && token->load();
}

} // namespace core
} // namespace als
