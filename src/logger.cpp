#include "sensorguard/logger.hpp"

#include <cstdarg>
#include <ctime>
#include <sys/time.h>
#include <unistd.h>

namespace sg {

Logger& Logger::instance() {
    static Logger inst;
    return inst;
}

Logger::~Logger() {
    if (file_) std::fclose(file_);
}

bool Logger::openFile(const std::string& path) {
    std::lock_guard<std::mutex> l(mu_);
    file_ = std::fopen(path.c_str(), "a");
    return file_ != nullptr;
}

void Logger::log(LogLevel lvl, const char* fmt, ...) {
    if (lvl < level_) return;
    static const char* names[] = {"DEBUG", "INFO", "WARN", "ERROR"};

    char msg[512];
    va_list ap;
    va_start(ap, fmt);
    std::vsnprintf(msg, sizeof msg, fmt, ap);
    va_end(ap);

    timeval tv{};
    gettimeofday(&tv, nullptr);
    std::tm tm{};
    localtime_r(&tv.tv_sec, &tm);
    char ts[32];
    std::strftime(ts, sizeof ts, "%H:%M:%S", &tm);

    std::lock_guard<std::mutex> l(mu_);
    if (!quiet_ || lvl >= LogLevel::Warn)
        std::fprintf(stderr, "%s.%03d [%s] %s\n", ts, static_cast<int>(tv.tv_usec / 1000), names[static_cast<int>(lvl)], msg);
    if (file_) {
        std::fprintf(file_, "%s.%03d [%s] %s\n", ts, static_cast<int>(tv.tv_usec / 1000), names[static_cast<int>(lvl)], msg);
        std::fflush(file_);
    }
}

}  // namespace sg
