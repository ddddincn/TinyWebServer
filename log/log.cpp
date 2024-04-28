#include "log.h"

Log::Log() {
    lineCount_ = 0;
    today_ = 0;
    isClose_ = true;
    isAsync_ = false;  // 默认同步写
    fp_ = nullptr;
    que_ = nullptr;
    writeThread_ = nullptr;
}

Log::~Log() {
    if (writeThread_ && writeThread_->joinable()) {
        while (!que_->empty()) {
            que_->flush();
        }
        que_->close();
        writeThread_->join();
    }
    if (fp_) {
        std::lock_guard<std::mutex> lock(mtx_);
        flush();
        fclose(fp_);
    }
}

void Log::init(int level, const char *path, const char *suffix, int maxCapacity) {
    level_ = level;
    path_ = path;
    suffix_ = suffix;
    isClose_ = false;
    lineCount_ = 0;

    if (maxCapacity > 0)  // 表示使用异步写入
    {
        isAsync_ = true;
        if (!que_) {
            std::unique_ptr<BlockQueue<std::string>> newQue(new BlockQueue<std::string>);
            que_ = std::move(newQue);
            std::unique_ptr<std::thread> newThread(new std::thread(&Log::asyncWrite, this));
            writeThread_ = std::move(newThread);
        }
    } else {
        isAsync_ = false;
    }

    time_t timer = time(nullptr);
    struct tm *systime = std::localtime(&timer);
    struct tm t = *systime;
    char fileName[MAX_NAME_LEN];
    snprintf(fileName, MAX_NAME_LEN - 1, "%s/%04d_%02d_%02d%s",
             path_, t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, suffix_);  // 文件名格式例子：./log/2024_01_01.log
    today_ = t.tm_mday;
    {
        std::lock_guard<std::mutex> locker(mtx_);
        if (fp_)  // 如果fp_!=nullptr说明fp_已经被初始化，刚刚已经使用过日志，需要将刚刚要写入的日志全部刷入到文件中
        {
            flush();
            fclose(fp_);
        }
        fp_ = fopen(fileName, "a");  // 打开新的log文件
        if (fp_ == nullptr)          // 如果fp_ == nullptr说明不存在日志文件，需要新建文件
        {
            mkdir(path_, 0777);
            fp_ = fopen(fileName, "a");
        }
        assert(fp_ != nullptr);
    }
}

Log *Log::instance() {
    static Log inst;
    return &inst;
}

void Log::flushLogThread() {
    Log::instance()->asyncWrite();
}

void Log::write(int leve, const char *format, ...) {
    struct timeval now;  // 这个结构体可以获取精度更高的时间
    gettimeofday(&now, nullptr);
    time_t tSec = now.tv_sec;
    struct tm *sysTime = localtime(&tSec);
    struct tm t = *sysTime;
    va_list vaList;

    if (today_ != t.tm_mday || (lineCount_ && (lineCount_ % MAX_LINES == 0)))  // 日志记录不是初始化时间，或达到日志最大行数需要新建文件
    {
        std::unique_lock<std::mutex> locker(mtx_);
        locker.unlock();

        char newFile[MAX_NAME_LEN];
        if (today_ != t.tm_mday)  // 日志记录不是初始化时间
        {
            snprintf(newFile, MAX_NAME_LEN - 1, "%s/%04d_%02d_%02d%s",
                     path_, t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, suffix_);
            today_ = t.tm_mday;
            lineCount_ = 0;
        } else {
            snprintf(newFile, MAX_NAME_LEN - 1, "%s/%04d_%02d_%02d-%d%s",
                     path_, t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, (lineCount_ / MAX_LINES), suffix_);
        }

        locker.lock();
        flush();      // 确保上一个日志完全写入
        fclose(fp_);  // 关闭之前的文件
        fp_ = fopen(newFile, "a");
        assert(fp_ != nullptr);
    }

    {  // 开始写入日志
        std::unique_lock<std::mutex> locker(mtx_);
        lineCount_++;
        int n = snprintf(buff_.beginWrite(), 128, "%04d-%02d-%02d %02d:%02d:%02d.%06ld",
                         t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec, now.tv_usec);  // 先写入时间
        buff_.hasWritten(n);
        appendLogLevelTitle(level_);

        va_start(vaList, format);
        int m = vsnprintf(buff_.beginWrite(), buff_.writableBytes(), format, vaList);
        va_end(vaList);
        buff_.hasWritten(m);
        buff_.append("\n\0", 2);

        if (isAsync_ && que_)  // 异步写入，如果阻塞队列未满就将缓存区的日志放入阻塞队列
        {
            locker.unlock();
            que_->push(buff_.retrieveAllToStr());
            locker.lock();
        } else  // 同步写入
        {
            fputs(buff_.peek(), fp_);
        }
        buff_.retrieveAll();
    }
}

void Log::flush() {
    if (isAsync_) {
        que_->flush();
    }
    fflush(fp_);
}

int Log::getLevel() {
    std::lock_guard<std::mutex> locker(mtx_);
    return level_;
}

void Log::setLevel(int level) {
    std::lock_guard<std::mutex> locker(mtx_);
    level_ = level;
}

bool Log::isClose() {
    std::lock_guard<std::mutex> locker(mtx_);
    return isClose_;
}

void Log::appendLogLevelTitle(int level) {
    switch (level) {
        case 0:
            buff_.append("[debug]: ", 9);
            break;
        case 1:
            buff_.append("[info] : ", 9);
            break;
        case 2:
            buff_.append("[warn] : ", 9);
            break;
        case 3:
            buff_.append("[error]: ", 9);
            break;
        default:
            buff_.append("[info] : ", 9);
            break;
    }
}

void Log::asyncWrite() {
    std::string str;
    while (que_->pop(str)) {
        std::lock_guard<std::mutex> locker(mtx_);
        fputs(str.c_str(), fp_);
    }
}