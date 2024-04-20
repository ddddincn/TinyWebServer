#ifndef BLOCKQUEUE_H
#define BLOCKQUEUE_H

#include <mutex>
#include <queue>
#include <cassert>
#include <condition_variable>

template <typename T>
class BlockQueue
{
public:
    explicit BlockQueue(size_t maxCapacity);
    ~BlockQueue();

    void clear();
    bool empty();
    bool full();
    void flush();
    void close();

    void push(const T &item); // 队尾添加元素
    bool pop(T &item);        // 队首删除元素
    T front();                // 返回队首元素
    T back();                 // 返回队尾元素

    size_t size();
    size_t capacity();

private:
    std::queue<T> que_;
    std::mutex mtx_;
    size_t capacity_;
    bool isClose_;
    std::condition_variable condConsumer_;
    std::condition_variable condProducer_;
};

template <typename T>
BlockQueue<T>::BlockQueue(size_t maxCapacity = 1024) : capacity_(maxCapacity)
{
    assert(maxCapacity > 0);
    isClose_ = false;
}

template <typename T>
BlockQueue<T>::~BlockQueue()
{
    std::lock_guard<std::mutex> locker(mtx_);
    std::queue<T>().swap(que_); // 清空队列操作
    isClose_ = true;
    condConsumer_.notify_all();
    condProducer_.notify_all();
}

template <typename T>
void BlockQueue<T>::clear()
{
    std::lock_guard<std::mutex> locker(mtx_);
    std::queue<T>().swap(que_); // 清空队列操作
}

template <typename T>
bool BlockQueue<T>::empty()
{
    std::lock_guard<std::mutex> locker(mtx_);
    return que_.empty();
}

template <typename T>
bool BlockQueue<T>::full()
{
    std::lock_guard<std::mutex> locker(mtx_);
    return que_.size() > capacity_;
}

template <typename T>
void BlockQueue<T>::flush()
{
    condConsumer_.notify_one();
}

template <typename T>
void BlockQueue<T>::close()
{
    std::lock_guard<std::mutex> locker(mtx_);
    std::queue<T>().swap(que_); // 清空队列操作
    isClose_ = true;
    condConsumer_.notify_all();
    condProducer_.notify_all();
}

template <typename T>
void BlockQueue<T>::push(const T &item) // push()由生产者调用
{
    std::unique_lock<std::mutex> locker(mtx_);
    while (que_.size() >= capacity_)
    {
        condProducer_.wait(locker); // 如果队列满了，生产者会阻塞
    }
    que_.push(item);
    condConsumer_.notify_one();
}

template <typename T>
bool BlockQueue<T>::pop(T &item) // pop()由消费者调用
{
    std::unique_lock<std::mutex> locker(mtx_);
    while (que_.empty())
    {
        if (isClose_)
        {
            return false;
        }
        condConsumer_.wait(locker); // 如果队列为空，消费者会等待生产者唤醒(当生产者push()成功时会唤醒)
    }
    item = que_.front();
    que_.pop();
    condProducer_.notify_one(); //
    return true;
}

template <typename T>
T BlockQueue<T>::front()
{
    std::lock_guard<std::mutex> locker(mtx_);
    return que_.front();
}

template <typename T>
T BlockQueue<T>::back()
{
    std::lock_guard<std::mutex> locker(mtx_);
    return que_.back();
}

template <typename T>
size_t BlockQueue<T>::size()
{
    std::lock_guard<std::mutex> locker(mtx_);
    return que_.size();
}

template <typename T>
size_t BlockQueue<T>::capacity()
{
    std::lock_guard<std::mutex> locker(mtx_);
    return capacity_;
}

#endif // BLOCKQUEUE_H