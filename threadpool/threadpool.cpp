#include "threadpool.h"

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
