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

ThreadPool::ThreadPool(size_t threadNum) : pool_(std::make_shared<Pool>()) {
    assert(threadNum > 0);
    for (size_t i = 0; i < threadNum; i++) {
        std::thread([pool = pool_]() {
                std::unique_lock<std::mutex> locker(pool->mtx);
                while (!pool->isClose)
                {
                    if (!pool->tasks.empty())
                    {
                        auto task = std::move(pool->tasks.front());
                        pool->tasks.pop();
                        locker.unlock();
                        task();
                        locker.lock();
                    }
                    else
                    {
                        pool->cond.wait(locker);
                    }
                } }).detach();
    }
}

ThreadPool::~ThreadPool() {
    if (static_cast<bool>(pool_)) {
        {
            std::lock_guard<std::mutex> locker(pool_->mtx);
            pool_->isClose = true;
        }
        pool_->cond.notify_all();
    }
}

template <typename T>
void ThreadPool::addTask(T &&task) {
    {
        std::lock_guard<std::mutex> locker(pool_->mtx);
        pool_->tasks.emplace(std::forward<T>(task));  // 这里使用完美转发
    }
    pool_->cond.notify_one();
}

#endif  // THREADPOOL_H
