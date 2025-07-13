/**
 * @file ThreadPool.h
 * @brief Thread pool implementation for concurrent task execution
 */

#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>
#include <chrono>

namespace als {
namespace core {

/**
 * @brief Thread pool for managing worker threads and task execution
 * 
 * Provides:
 * - Fixed-size thread pool with work queues
 * - Task prioritization (urgent vs. background tasks)
 * - Cancellation support with atomic flags
 * - Thread-safe task submission and completion callbacks
 */
class ThreadPool {
public:
    /**
     * @brief Task priority levels
     */
    enum class TaskPriority {
        LOW = 0,      ///< Background tasks (indexing, cleanup)
        NORMAL = 1,   ///< Regular LSP requests
        HIGH = 2,     ///< User-interactive requests (completion, hover)
        URGENT = 3    ///< Critical requests (shutdown, cancellation)
    };

    /**
     * @brief Task statistics
     */
    struct TaskStats {
        size_t submitted = 0;
        size_t completed = 0;
        size_t cancelled = 0;
        size_t failed = 0;
        std::chrono::milliseconds total_execution_time{0};
        std::chrono::milliseconds average_execution_time{0};
    };

    /**
     * @brief Construct thread pool
     * @param num_threads Number of worker threads (default: hardware concurrency)
     * @param max_queue_size Maximum number of queued tasks (default: 1000)
     */
    explicit ThreadPool(size_t num_threads = std::thread::hardware_concurrency(),
                       size_t max_queue_size = 1000);

    /**
     * @brief Destructor - waits for all tasks to complete
     */
    ~ThreadPool();

    // Non-copyable, non-movable
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool(ThreadPool&&) = delete;
    ThreadPool& operator=(ThreadPool&&) = delete;

    /**
     * @brief Submit a task with priority
     * @param priority Task priority level
     * @param f Function to execute
     * @param args Function arguments
     * @return Future for the task result
     */
    template<typename F, typename... Args>
    auto submit(TaskPriority priority, F&& f, Args&&... args) 
        -> std::future<typename std::invoke_result_t<F, Args...>>;

    /**
     * @brief Submit a task with normal priority
     * @param f Function to execute
     * @param args Function arguments
     * @return Future for the task result
     */
    template<typename F, typename... Args>
    auto submit(F&& f, Args&&... args) 
        -> std::future<typename std::invoke_result_t<F, Args...>>;

    /**
     * @brief Submit a task with cancellation support
     * @param priority Task priority level
     * @param cancellation_token Cancellation token to check
     * @param f Function to execute
     * @param args Function arguments
     * @return Future for the task result
     */
    template<typename F, typename... Args>
    auto submitCancellable(TaskPriority priority, 
                          std::shared_ptr<std::atomic<bool>> cancellation_token,
                          F&& f, Args&&... args) 
        -> std::future<typename std::invoke_result_t<F, Args...>>;

    /**
     * @brief Create a cancellation token
     * @return Shared cancellation token
     */
    std::shared_ptr<std::atomic<bool>> createCancellationToken();

    /**
     * @brief Cancel all pending tasks
     */
    void cancelAllTasks();

    /**
     * @brief Wait for all current tasks to complete
     * @param timeout Maximum time to wait
     * @return true if all tasks completed within timeout
     */
    bool waitForCompletion(std::chrono::milliseconds timeout = std::chrono::milliseconds::max());

    /**
     * @brief Resize the thread pool
     * @param num_threads New number of threads
     */
    void resize(size_t num_threads);

    // Status and monitoring
    size_t size() const { return workers_.size(); }
    size_t activeThreads() const { return active_threads_.load(); }
    size_t queuedTasks() const;
    TaskStats getStats() const;
    void resetStats();

private:
    /**
     * @brief Internal task representation
     */
    struct Task {
        std::function<void()> function;
        TaskPriority priority;
        std::chrono::steady_clock::time_point submit_time;
        std::shared_ptr<std::atomic<bool>> cancellation_token;
        
        // Priority queue comparison (higher priority first)
        bool operator<(const Task& other) const {
            if (priority != other.priority) {
                return priority < other.priority; // Higher priority first
            }
            return submit_time > other.submit_time; // Earlier submission first
        }
    };

    // Worker threads
    std::vector<std::thread> workers_;
    
    // Task queue with priority
    std::priority_queue<Task> task_queue_;
    mutable std::mutex queue_mutex_;
    std::condition_variable condition_;
    
    // Thread pool state
    std::atomic<bool> stop_;
    std::atomic<size_t> active_threads_;
    size_t max_queue_size_;
    
    // Statistics
    mutable std::mutex stats_mutex_;
    TaskStats stats_;
    
    // Worker thread function
    void workerLoop();
    
    // Helper to check if task should be cancelled
    bool shouldCancel(const std::shared_ptr<std::atomic<bool>>& token) const;
};

// Template implementations
template<typename F, typename... Args>
auto ThreadPool::submit(TaskPriority priority, F&& f, Args&&... args) 
    -> std::future<typename std::invoke_result_t<F, Args...>> {
    return submitCancellable(priority, nullptr, std::forward<F>(f), std::forward<Args>(args)...);
}

template<typename F, typename... Args>
auto ThreadPool::submit(F&& f, Args&&... args) 
    -> std::future<typename std::invoke_result_t<F, Args...>> {
    return submit(TaskPriority::NORMAL, std::forward<F>(f), std::forward<Args>(args)...);
}

template<typename F, typename... Args>
auto ThreadPool::submitCancellable(TaskPriority priority, 
                                  std::shared_ptr<std::atomic<bool>> cancellation_token,
                                  F&& f, Args&&... args) 
    -> std::future<typename std::invoke_result_t<F, Args...>> {
    using return_type = typename std::invoke_result_t<F, Args...>;
    
    auto task = std::make_shared<std::packaged_task<return_type()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...)
    );
    
    std::future<return_type> result = task->get_future();
    
    {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        
        if (stop_) {
            throw std::runtime_error("ThreadPool is stopped");
        }
        
        if (task_queue_.size() >= max_queue_size_) {
            throw std::runtime_error("ThreadPool queue is full");
        }
        
        auto start_time = std::chrono::steady_clock::now();
        
        task_queue_.emplace(Task{
            [task, cancellation_token, start_time, this]() {
                if (shouldCancel(cancellation_token)) {
                    std::lock_guard<std::mutex> stats_lock(stats_mutex_);
                    stats_.cancelled++;
                    return;
                }
                
                try {
                    (*task)();
                    
                    auto end_time = std::chrono::steady_clock::now();
                    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                        end_time - start_time);
                    
                    std::lock_guard<std::mutex> stats_lock(stats_mutex_);
                    stats_.completed++;
                    stats_.total_execution_time += duration;
                    if (stats_.completed > 0) {
                        stats_.average_execution_time = stats_.total_execution_time / stats_.completed;
                    }
                } catch (...) {
                    std::lock_guard<std::mutex> stats_lock(stats_mutex_);
                    stats_.failed++;
                }
            },
            priority,
            std::chrono::steady_clock::now(),
            cancellation_token
        });
        
        stats_.submitted++;
    }
    
    condition_.notify_one();
    return result;
}

} // namespace core
} // namespace als
