#pragma once
#include <iostream>
#include <vector>
#include <thread>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <map>
#include <random>

enum ThreadType {
    RANDOM_THREAD = -1,
    METRICS_MANAGER_THREAD = 0,
};

class ThreadManager {
public:
    static ThreadManager& GetInstance(int poolSize) {
        static ThreadManager instance(poolSize);
        return instance;
    }

    ~ThreadManager() {
        // Wait for all threads to finish
        for (std::thread& thread : threads) {
            thread.join();
        }
    }

    // Add a task to a specific thread in the thread pool
    // Use threadIndex = -1 to indicate that the task can run on any available thread
    void AddTaskToThread(std::function<void()> task, int threadIndex = ThreadType::RANDOM_THREAD) {
        if (threadIndex == ThreadType::RANDOM_THREAD) {
            // If threadIndex is -1, randomly select an available thread.
            // We however do not want to use the thread reserved for Metrics
            // manager for other tasks.
            std::uniform_int_distribution<int> distribution(ThreadType::METRICS_MANAGER_THREAD + 1, poolSize - 1);
            threadIndex = distribution(generator);
        }

        if (threadIndex >= 0 && threadIndex < poolSize) {
            std::lock_guard<std::mutex> lock(tasksMutex);
            tasks[threadIndex].emplace_back(task);
            condition.notify_all();
        }
        else {
            std::cout << "Invalid thread index." << std::endl;
        }
    }

    void Stop() {
        should_stop.store(true);
    }

private:
    int poolSize;
    std::atomic<bool> should_stop;
    std::vector<std::thread> threads;
    std::map<int, std::vector<std::function<void()>>> tasks;
    std::mutex tasksMutex;
    std::condition_variable condition;
    std::default_random_engine generator; // Random number generator

    ThreadManager(int poolSize) : poolSize(poolSize) {
        should_stop.store(false);
        // Initialize the thread pool
        for (int i = 0; i < poolSize; ++i) {
            threads.emplace_back(std::bind(&ThreadManager::ThreadFunction, this, i));
        }
    }
    ThreadManager() = delete;
    ThreadManager(const ThreadManager&) = delete;
    ThreadManager& operator=(const ThreadManager&) = delete;

    // Function executed by each thread in the pool
    void ThreadFunction(int id) {
        std::mutex mutex;

        while (!should_stop.load()) {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lock(mutex);

                // Wait for a task if the task queue is empty
                condition.wait(lock, [this, id] {
                    // If we have a signal to exit, we should stop waiting
                    if (should_stop.load()) {
                        return true;
                    }

                    return !tasks[id].empty();
                });

                // Get the next task from this thread's task queue
                if (!should_stop.load() && !tasks[id].empty()) {
                    std::lock_guard<std::mutex> lockTasks(tasksMutex);
                    task = tasks[id].front();
                    tasks[id].erase(tasks[id].begin());
                }

                lock.unlock();
            }

            // Execute the task
            if (task) {
                task();
            }
        }

        // This will trigger all pending threads waiting when the loop
        // has been triggered to stop.
        condition.notify_all();
    }
};
