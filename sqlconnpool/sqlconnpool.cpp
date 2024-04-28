#include "sqlconnpool.h"

SqlConnPool::~SqlConnPool() {
    close();
}

SqlConnPool *SqlConnPool::instance() {
    static SqlConnPool inst;
    return &inst;
}

void SqlConnPool::init(const char *host, int port, const char *user, const char *passwd, const char *db, int connSize) {
    assert(connSize > 0);
    MAX_CONN_ = connSize;
    for (int i = 0; i < connSize; i++) {
        MYSQL *conn = nullptr;
        conn = mysql_init(conn);
        if (!conn) {
            LOG_ERROR("MySql Init Error!");
            assert(conn);
        }
        conn = mysql_real_connect(conn, host, user, passwd, db, port, nullptr, 0);
        if (!conn) {
            LOG_ERROR("MySql Connect Error!");
        }
        connQue_.push(conn);
        sem_init(&semId_, 0, MAX_CONN_);
    }
}

MYSQL *SqlConnPool::getConn() {
    MYSQL *conn = nullptr;
    if (connQue_.empty()) {
        LOG_WARN("SqlConnPool Is Busy!");
    }
    sem_wait(&semId_);
    {
        std::lock_guard<std::mutex> locker(mtx_);
        conn = connQue_.front();
        connQue_.pop();
    }
    return conn;
}

void SqlConnPool::freeConn(MYSQL *conn) {
    assert(conn);
    std::lock_guard<std::mutex> locker(mtx_);
    connQue_.push(conn);
    sem_post(&semId_);
}

int SqlConnPool::getFreeConnCount() {
    std::lock_guard<std::mutex> locker(mtx_);
    return connQue_.size();
}

void SqlConnPool::close() {
    std::lock_guard<std::mutex> locker(mtx_);
    while (!connQue_.empty()) {
        auto conn = connQue_.front();
        connQue_.pop();
        mysql_close(conn);
    }
    mysql_library_end();
}