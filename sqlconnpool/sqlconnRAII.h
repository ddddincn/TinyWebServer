#ifndef SQLCONNRAII_H
#define SQLCONNRAII_H

#include "sqlconnpool.h"

class SqlConnRAII {
public:
    SqlConnRAII(MYSQL **conn, SqlConnPool *connPool) {
        assert(connPool);
        *conn = connPool->getConn();
        conn_ = *conn;
        connPool_ = connPool;
    }

    ~SqlConnRAII() {
        if (conn_) {
            connPool_->freeConn(conn_);
        }
    }

private:
    MYSQL *conn_;
    SqlConnPool *connPool_;
};

#endif  // SQLCONNRAII_H