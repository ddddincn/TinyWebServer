#include "heaptimer.h"

HeapTimer::HeapTimer() {
    heap_.reserve(64);
}

HeapTimer::~HeapTimer() {
    clear();
}

void HeapTimer::adjust(int id, int timeout) {  // 调整指定的id节点
    assert(!heap_.empty() && ref_.count(id) > 0);
    heap_[ref_[id]].expires = Clock::now() + MS(timeout);
    siftdown_(id, heap_.size());
}

void HeapTimer::add(int id, int timeout, const TimeoutCallBack& cb) {
    assert(id >= 0);
    size_t i;
    if (ref_.count(id) == 0) {  // 插入新节点
        i = heap_.size();
        ref_[id] = i;
        heap_.push_back({id, Clock::now() + MS(timeout), cb});
        siftup_(i);
    } else {  // 已存在该节点，调整堆
        i = ref_[id];
        heap_[i].expires = Clock::now() + MS(timeout);
        heap_[i].cb = cb;
        if (!siftdown_(i, heap_.size())) {
            siftup_(i);
        }
    }
}

void HeapTimer::doWork(int id) {
    if (heap_.empty() || ref_.count(id) == 0) {
        return;
    }
    size_t i = id;
    TimerNode node = heap_[i];
    node.cb();
    del_(i);
}

void HeapTimer::clear() {
    ref_.clear();
    heap_.clear();
}

void HeapTimer::tick() {
    if (heap_.empty()) {
        return;
    }
    while (!heap_.empty()) {
        TimerNode node = heap_.front();
        if (std::chrono::duration_cast<MS>(node.expires - Clock::now()).count() > 0) {
            break;
        }
        node.cb();
        pop();
    }
}

void HeapTimer::pop() {
    assert(!heap_.empty());
    del_(0);
}

int getNextTick() {
}

void HeapTimer::del_(size_t i) {
    // 删除指定节点
    assert(!heap_.empty() && i >= 0 && i < heap_.size());
    size_t n = heap_.size() - 1;
    assert(i <= n);
    if (i < n) {
        swapNode_(i, n);
        if (!siftdown_(i, n)) {
            siftup_(i);
        }
    }
    ref_.erase(heap_.back().id);
    heap_.pop_back();
}

void HeapTimer::siftup_(size_t i) {
    assert(i >= 0 && i < heap_.size());
    size_t j = (i - 1) / 2;  // 父节点下标
    while (j >= 0) {
        if (heap_[j] < heap_[i]) {
            break;
        }
        swapNode_(i, j);
        i = j;
        j = (i - 1) / 2;
    }
}

bool HeapTimer::siftdown_(size_t index, size_t n) {
    assert(index >= 0 && index < heap_.size());
    assert(n >= 0 && n < heap_.size());
    size_t i = index;
    size_t j = i * 2 + 1;  // 子节点下标
    while (j < n) {
        if (j + 1 < n && heap_[j + 1] < heap_[j]) {
            j++;
        }
        if (heap_[i] < heap_[j]) {
            break;
        }
        swapNode_(i, j);
        i = j;
        j = i * 2 + 1;
    }
    return i > index;
}

void HeapTimer::swapNode_(size_t i, size_t j) {
    assert(i >= 0 && j < heap_.size());
    assert(j >= 0 && j < heap_.size());
    std::swap(heap_[i], heap_[j]);
    ref_[heap_[i].id] = i;
    ref_[heap_[j].id] = j;
}