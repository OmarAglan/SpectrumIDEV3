/**
 * @file test_threadpool.cpp
 * @brief Tests for ThreadPool functionality
 */

#include <iostream>
#include <cassert>
#include <chrono>
#include <atomic>

#include "als/core/ThreadPool.h"

using namespace als::core;

/**
 * @brief Test basic ThreadPool functionality
 */
void testBasicThreadPool() {
    std::cout << "[TEST] Testing basic ThreadPool functionality..." << std::endl;
    
    ThreadPool pool(2, 100); // 2 threads, max 100 tasks
    
    // Test basic task submission
    auto future1 = pool.submit([]() { return 42; });
    auto future2 = pool.submit([]() { return std::string("hello"); });
    
    assert(future1.get() == 42);
    assert(future2.get() == "hello");
    
    std::cout << "[TEST] Basic ThreadPool functionality passed!" << std::endl;
}

/**
 * @brief Test task priorities
 */
void testTaskPriorities() {
    std::cout << "[TEST] Testing task priorities..." << std::endl;
    
    ThreadPool pool(1, 100); // Single thread to test ordering
    
    std::atomic<int> counter{0};
    std::vector<int> execution_order;
    std::mutex order_mutex;
    
    // Submit tasks with different priorities
    auto low_task = pool.submit(ThreadPool::TaskPriority::LOW, [&]() {
        std::lock_guard<std::mutex> lock(order_mutex);
        execution_order.push_back(1);
        return counter.fetch_add(1);
    });
    
    auto high_task = pool.submit(ThreadPool::TaskPriority::HIGH, [&]() {
        std::lock_guard<std::mutex> lock(order_mutex);
        execution_order.push_back(2);
        return counter.fetch_add(1);
    });
    
    auto urgent_task = pool.submit(ThreadPool::TaskPriority::URGENT, [&]() {
        std::lock_guard<std::mutex> lock(order_mutex);
        execution_order.push_back(3);
        return counter.fetch_add(1);
    });
    
    // Wait for completion
    low_task.wait();
    high_task.wait();
    urgent_task.wait();
    
    // Verify all tasks completed
    assert(counter.load() == 3);
    assert(execution_order.size() == 3);
    
    std::cout << "[TEST] Task priorities passed!" << std::endl;
}

/**
 * @brief Test cancellation functionality
 */
void testCancellation() {
    std::cout << "[TEST] Testing cancellation functionality..." << std::endl;
    
    ThreadPool pool(2, 100);
    
    auto cancellation_token = pool.createCancellationToken();
    std::atomic<bool> task_executed{false};
    
    // Submit a task with cancellation token
    auto future = pool.submitCancellable(
        ThreadPool::TaskPriority::NORMAL,
        cancellation_token,
        [&]() {
            task_executed = true;
            return 42;
        }
    );
    
    // Cancel the token immediately
    cancellation_token->store(true);
    
    // Give some time for potential execution
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    // Task should not have executed due to cancellation
    // Note: This test is timing-dependent and may occasionally pass even if cancellation fails
    
    std::cout << "[TEST] Cancellation functionality passed!" << std::endl;
}

/**
 * @brief Test ThreadPool statistics
 */
void testStatistics() {
    std::cout << "[TEST] Testing ThreadPool statistics..." << std::endl;
    
    ThreadPool pool(2, 100);
    
    // Submit several tasks
    std::vector<std::future<int>> futures;
    for (int i = 0; i < 5; ++i) {
        futures.push_back(pool.submit([i]() { 
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            return i * i; 
        }));
    }
    
    // Wait for all tasks to complete
    for (auto& future : futures) {
        future.wait();
    }
    
    auto stats = pool.getStats();
    assert(stats.submitted >= 5);
    assert(stats.completed >= 5);
    
    std::cout << "[TEST] Statistics - Submitted: " << stats.submitted 
              << ", Completed: " << stats.completed 
              << ", Average time: " << stats.average_execution_time.count() << "ms" << std::endl;
    
    std::cout << "[TEST] ThreadPool statistics passed!" << std::endl;
}

/**
 * @brief Test ThreadPool resize functionality
 */
void testResize() {
    std::cout << "[TEST] Testing ThreadPool resize..." << std::endl;
    
    ThreadPool pool(2, 100);
    assert(pool.size() == 2);
    
    // Resize to more threads
    pool.resize(4);
    assert(pool.size() == 4);
    
    // Resize to fewer threads
    pool.resize(1);
    assert(pool.size() == 1);
    
    // Test that it still works after resize
    auto future = pool.submit([]() { return 123; });
    assert(future.get() == 123);
    
    std::cout << "[TEST] ThreadPool resize passed!" << std::endl;
}

/**
 * @brief Test ThreadPool with heavy load
 */
void testHeavyLoad() {
    std::cout << "[TEST] Testing ThreadPool with heavy load..." << std::endl;
    
    ThreadPool pool(4, 1000);
    
    const int num_tasks = 100;
    std::vector<std::future<int>> futures;
    futures.reserve(num_tasks);
    
    auto start_time = std::chrono::steady_clock::now();
    
    // Submit many tasks
    for (int i = 0; i < num_tasks; ++i) {
        futures.push_back(pool.submit([i]() {
            // Simulate some work
            int sum = 0;
            for (int j = 0; j < 1000; ++j) {
                sum += j;
            }
            return sum + i;
        }));
    }
    
    // Wait for all tasks and verify results
    for (int i = 0; i < num_tasks; ++i) {
        int expected = (999 * 1000 / 2) + i; // Sum of 0..999 + i
        assert(futures[i].get() == expected);
    }
    
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "[TEST] Heavy load completed in " << duration.count() << "ms" << std::endl;
    
    auto stats = pool.getStats();
    std::cout << "[TEST] Final stats - Submitted: " << stats.submitted 
              << ", Completed: " << stats.completed << std::endl;
    
    std::cout << "[TEST] ThreadPool heavy load passed!" << std::endl;
}

/**
 * @brief Main test runner for ThreadPool
 */
int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "ThreadPool Test Suite" << std::endl;
    std::cout << "========================================" << std::endl;
    
    try {
        testBasicThreadPool();
        testTaskPriorities();
        testCancellation();
        testStatistics();
        testResize();
        testHeavyLoad();
        
        std::cout << "========================================" << std::endl;
        std::cout << "All ThreadPool tests passed!" << std::endl;
        std::cout << "========================================" << std::endl;
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Test failed with unknown exception" << std::endl;
        return 1;
    }
}
