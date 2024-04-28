#ifndef SQLCONNPOOL_H
#define SQLCONNPOOL_H

#include <mysql/mysql.h>
#include <mutex>
#include <semaphore.h>
#include <thread>
#include <queue>
#include <cassert>

#include "../log/log.h"

class SqlConnPool
{
public:
    static SqlConnPool *instance();
    void init(const char *host, int port, const char *user, const char *pwd, const char *dbName, int connSize = 16);

    MYSQL *getConn();
    void freeConn(MYSQL *conn);
    int getFreeConnCount();
    void close();

private:
    SqlConnPool() = default;
    ~SqlConnPool();

private:
    int MAX_CONN_;

    std::queue<MYSQL *> connQue_;
    std::mutex mtx_;
    sem_t semId_;
};

#endif // SQLCONNPOOL_H