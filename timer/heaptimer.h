#ifndef HEAP_TIMER_H
#define HEAP_TIMER_H

#include <cassert>
#include <chrono>
#include <functional>
#include <unordered_map>
#include <vector>

using TimeoutCallBack = std::function<void()>;
using Clock = std::chrono::high_resolution_clock;
using MS = std::chrono::milliseconds;
using TimeStamp = Clock::time_point;

struct TimerNode {
    int id;
    TimeStamp expires;
    TimeoutCallBack cb;
    bool operator<(const TimerNode& t) {
        return expires < t.expires;
    }
};

class HeapTimer {
public:
    HeapTimer();
    ~HeapTimer();

    void adjust(int id, int timeout);
    void add(int id, int timeout, const TimeoutCallBack& cb);
    void doWork(int id);
    void clear();
    void tick();
    void pop();

    int getNextTick();

private:
    void del_(size_t i);
    void siftup_(size_t i);
    bool siftdown_(size_t i, size_t n);
    void swapNode_(size_t i, size_t j);

private:
    std::vector<TimerNode> heap_;
    std::unordered_map<int, size_t> ref_;  // id到vector下标索引
};

#endif  // HEAP_TIMER_H