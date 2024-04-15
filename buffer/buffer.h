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
    Buffer();
    ~Buffer();

    size_t readableBytes() const;
    size_t writeableBytes() const;
    size_t prependableBytes() const;

private:
    std::vector<char> buff_;
    std::atomic<size_t> readPos_;
    std::atomic<size_t> writePos_;
};

#endif // BUFFER_H