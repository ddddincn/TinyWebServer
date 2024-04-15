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
    Buffer(int initBuffSize = 1024); // 默认初始化长度1024
    ~Buffer() = default;             // 使用默认构造函数

    size_t readableBytes() const;    // 返回可读字节数
    size_t writeableBytes() const;   // 返回可写字节数
    size_t prependableBytes() const; // 返回可预留字节数(已读取过的数据)

    const char *peek() const; // 返回当前读取位置的指针

    void retrieve(size_t len); // 处理数据函数
    void retrieveUntil(const char *end);
    void retrieveAll();
    std::string retrieveAllToStr();

    void hasWriten(size_t len); // 更新写入位置
    void ensureWriteable(size_t len);

    char *beginWrite(); // 返回可写位置的指针
    const char *beginWriteConst() const;

    void append(const char *data, size_t len); // 向buffer中追加数据

    ssize_t readFd(int fd, int *saveErrno);
    ssize_t writeFd(int fd, int *saveErrno);

private:
    char *beginPtr_();
    const char *beginPtr_() const;
    void makeSpace_(size_t len);

    std::vector<char> buffer_;
    std::atomic<size_t> readPos_;
    std::atomic<size_t> writePos_;
};

#endif // BUFFER_H