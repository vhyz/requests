//
// Created by vhyz on 19-5-25.
//

#ifndef REQUESTS_LOGGING_H
#define REQUESTS_LOGGING_H

#include <string>
#include <cstdio>

#define hlog(level, ...)                                                                 \
do {                                                                                     \
    Logger::getLogger().logv(level, __FILE__, __LINE__, __func__, __VA_ARGS__);          \
} while(0)

#define trace(...) hlog(Logger::LTRACE, __VA_ARGS__)
#define debug(...) hlog(Logger::LDEBUG, __VA_ARGS__)
#define info(...) hlog(Logger::LINFO, __VA_ARGS__)
#define warn(...) hlog(Logger::LWARN, __VA_ARGS__)
#define error(...) hlog(Logger::LERROR, __VA_ARGS__)
#define fatal(...) hlog(Logger::LFATAL, __VA_ARGS__)

class Logger {
public:
    enum LogLevel {
        LFATAL = 0, LERROR, LWARN, LINFO, LDEBUG, LTRCE, LALL
    };

    Logger();

    ~Logger();

    static Logger& getLogger();

    static const char *levelStrings[LALL + 1];

    void setFileName(const std::string& fileName);
    
    void logv(LogLevel logLevel, const char *file, int line, const char *func, const char *fmt, ...);
private:

    int fd_;
    std::string fileName_;

};


#endif //REQUESTS_LOGGING_H
