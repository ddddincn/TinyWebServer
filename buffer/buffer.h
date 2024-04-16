#ifndef BUFFER_H
#define BUFFER_H

#include <iostream>
#include <vector>
#include <atomic>
#include <cassert>
#include <cstring>
#include <unistd.h>
#include <sys/uio.h> //readv

class Buffer
{
public:
    Buffer(int bufferSize = 1024);
    ~Buffer() = default;

    size_t readableBytes() const;    // 返回可写字节数
    size_t writableBytes() const;   // 返回可读字节数
    size_t prependableBytes() const; // 返回已读字节数

    const char *peek() const; // 返回读取位置的指针

    void retrieve(size_t len);
    void retrieveUntil(const char *end);
    void retrieveAll();
    std::string retrieveAllToStr();

    void hasWritten(size_t len);

    char *beginWrite();
    void ensureWriteable(size_t len);

    void append(const char *data, size_t len);

    ssize_t writeFd(int fd, int *saveErrno);
    ssize_t readFd(int fd, int *saveErrno);

private:
    char *beginPtr_();
    const char *beginPtr_() const;
    void makeSpace_(size_t len);

private:
    std::vector<char> buff_;
    std::atomic<size_t> readPos_;
    std::atomic<size_t> writePos_;
};

#endif // BUFFER_H