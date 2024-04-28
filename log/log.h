#ifndef LOG_H
#define LOG_H

#include <sys/stat.h>
#include <sys/time.h>

#include <cassert>
#include <cstdarg>
#include <mutex>
#include <thread>

#include "../buffer/buffer.h"
#include "blockqueue.h"

class Log {
   public:
    void init(int level, const char *path = "./log", const char *suffix = ".log", int maxCapacity = 1024);
    static Log *instance();  // 单例模式
    void flushLogThread();
    void write(int leve, const char *format, ...);
    void flush();

    int getLevel();
    void setLevel(int level);
    bool isClose();

   private:
    Log();
    ~Log();
    void appendLogLevelTitle(int level);
    void asyncWrite();

   private:
    static const int MAX_PATH_LEN = 256;
    static const int MAX_NAME_LEN = 256;
    static const int MAX_LINES = 50000;

    const char *path_;    // 路径
    const char *suffix_;  // 后缀

    int maxLines_;
    int lineCount_;
    int today_;  // 记录当天时间

    Buffer buff_;  // 缓存区
    int level_;

    bool isClose_;
    bool isAsync_;

    FILE *fp_;
    std::unique_ptr<BlockQueue<std::string>> que_;  // 阻塞队列，异步写入使用
    std::unique_ptr<std::thread> writeThread_;      // 异步写入线程
    std::mutex mtx_;
};
// 这里用do()while(0)来写宏是为了防止编译器歧义
#define LOG_BASE(level, format, ...)                       \
    do {                                                   \
        Log *log = Log::instance();                        \
        if (!log->isClose() && log->getLevel() <= level) { \
            log->write(level, format, ##__VA_ARGS__);      \
            log->flush();                                  \
        }                                                  \
    } while (0);

#define LOG_DEBUG(format, ...)             \
    do {                                   \
        LOG_BASE(0, format, ##__VA_ARGS__) \
    } while (0);
#define LOG_INFO(format, ...)              \
    do {                                   \
        LOG_BASE(1, format, ##__VA_ARGS__) \
    } while (0);
#define LOG_WARN(format, ...)              \
    do {                                   \
        LOG_BASE(2, format, ##__VA_ARGS__) \
    } while (0);
#define LOG_ERROR(format, ...)             \
    do {                                   \
        LOG_BASE(3, format, ##__VA_ARGS__) \
    } while (0);

#endif  // LOG_H