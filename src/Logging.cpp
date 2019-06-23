//
// Created by vhyz on 19-5-25.
//

#include "Logging.h"
#include <cstdarg>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <cstring>
#include <algorithm>

Logger::Logger() : fd_(-1) {}

Logger::~Logger() {
    if (fd_ != -1) {
        close(fd_);
    }
}

Logger &Logger::getLogger() {
    static Logger logger;
    return logger;
}

void Logger::logv(Logger::LogLevel logLevel, const char *file, int line, const char *func, const char *fmt, ...) {
    int fd = fd_ == -1 ? STDOUT_FILENO : fd_;
    char buf[4096];
    va_list vaList;
    char *p = buf;
    char *limit = buf + sizeof buf;

    p += snprintf(p, sizeof buf, "%s %d %s ", file, line, func);
    va_start(vaList, fmt);
    p += snprintf(p, limit - p, fmt, vaList);
    va_end(vaList);

    p = std::min(p, limit - 2);
    while (*--p == '\n') {}
    *++p = '\n';
    *++p = '\0';
    int err = write(fd, buf, p - buf);
    if (err < 0) {
        fprintf(stderr, "write log file %s failed. written %d errmsg: %s\n", fileName_.c_str(), err, strerror(errno));
    }
}

const char *Logger::levelStrings[LALL + 1] = {
        "FATAL", "ERROR", "WARN", "INFO", "DEBUG", "TRACE", "ALL"
};

void Logger::setFileName(const std::string &fileName) {
    int fd = open(fileName.c_str(), O_APPEND | O_CREAT | O_WRONLY | O_CLOEXEC);
    if (fd < 0) {
        fprintf(stderr, "open log file %s failed. msg: %s ignored\n", fileName.c_str(), strerror(errno));
        return;
    }
    fileName_ = fileName;
    if (fd_ != -1) {
        close(fd_);
    }
    fd_ = fd;
}
