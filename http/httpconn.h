#ifndef HTTPCONN_H
#define HTTPCONN_H

#include <arpa/inet.h>

#include <atomic>

#include "../buffer/buffer.h"
#include "../log/log.h"
#include "../sqlconnpool/sqlconnRAII.h"
#include "httprequest.h"
#include "httpresponse.h"

class HttpConn {
public:
    HttpConn();
    ~HttpConn();

    void init(int sockFd, const sockaddr_in &addr);

    ssize_t read(int *saveErrno);
    ssize_t write(int *saveErrno);

    void close();

    int getFd() const;
    int getPort() const;
    const char *getIP() const;
    sockaddr_in getAddr() const;

    bool process();

    int toWriteBytes();
    bool isKeepAlive() const;

    static bool isET;
    static const char *srcDir;
    static std::atomic<int> userCount;

private:
    int fd_;
    struct sockaddr_in addr_;

    bool isClose_;

    int iovCnt_;
    struct iovec iov_[2];

    Buffer readBuff_;   // 读缓存区
    Buffer writeBuff_;  // 写缓存区

    HttpRequest request_;
    HttpResponse response_;
};

#endif  // HTTPCONN_H