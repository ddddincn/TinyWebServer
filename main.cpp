#include <unistd.h>

#include "./webserver/webserver.h"

int main() {
    /* 守护进程 后台运行 */
    // daemon(1, 0);

    WebServer server(
        1316, 3, 60000, false,             /* 端口 ET模式 timeoutMs 优雅退出  */
        "47.76.100.96",3306, "root", "mysql_npCZbp", "webserver", /* Mysql配置 */
        12, 6, true, 1, 1024);             /* 连接池数量 线程池数量 日志开关 日志等级 日志异步队列容量 */
    server.start();
}
