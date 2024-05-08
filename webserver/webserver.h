#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cassert>
#include <unordered_map>

#include "../epoller/epoller.h"
#include "../http/httpconn.h"
#include "../log/log.h"
#include "../threadpool/threadpool.h"
#include "../timer/heaptimer.h"

class WebServer {
public:
    WebServer(int port, int trigMode, int timeout, bool optLinger,
              const char* sqlHost, int sqlPort, const char* sqlUser, const char* sqlPwd, const char* dbName,
              int connPollNum, int threadNum,
              bool openLog, int logLevel, int logQueSize);
    ~WebServer();

    void start();

private:
    static const int MAX_FD = 65535;
    static int setFdNonblock(int fd);

    bool initSocket_();
    void initEventMode_(int trigMode);
    void addClient(int fd, struct sockaddr_in addr);

    void dealListen_();
    void dealWrite_(HttpConn* client);
    void dealRead_(HttpConn* client);

    void sendError_(int fd, const char* info);
    void extentTime_(HttpConn* client);
    void closeConn_(HttpConn* client);

    void onRead_(HttpConn* client);
    void onWrite_(HttpConn* client);
    void onProcess(HttpConn* client);

private:
    int port_;
    bool openLinger_;
    int timeout_;
    bool isClose_;
    int listenFd_;
    char* srcDir_;

    uint32_t listenEvent_;
    uint32_t connEvent_;

    std::unique_ptr<HeapTimer> timer_;
    std::unique_ptr<ThreadPool> threadpool_;
    std::unique_ptr<Epoller> epoller_;
    std::unordered_map<int, HttpConn> users_;
};

#endif  // WEBSERVER_H