#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <cassert>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>

class ThreadPool {
public:
    explicit ThreadPool(size_t threadNum = 8);
    ThreadPool() = default;
    ThreadPool(ThreadPool &&) = default;
    ~ThreadPool();

    template <typename T>
    void addTask(T &&task);

private:
    struct Pool {
        std::queue<std::function<void()>> tasks;
        std::mutex mtx;
        std::condition_variable cond;
        bool isClose;
    };
    std::shared_ptr<Pool> pool_;
};
template <typename T>
void ThreadPool::addTask(T &&task) {
    {
        std::lock_guard<std::mutex> locker(pool_->mtx);
        pool_->tasks.emplace(std::forward<T>(task));  // 这里使用完美转发
    }
    pool_->cond.notify_one();
}

#endif  // THREADPOOL_H
