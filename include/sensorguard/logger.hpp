#pragma once
// Minimal thread-safe logger (stderr + optional file, fflush on every line).
#include <cstdio>
#include <mutex>
#include <string>

namespace sg {

enum class LogLevel { Debug = 0, Info, Warn, Error };

class Logger {
public:
    static Logger& instance();
    void setLevel(LogLevel l) { level_ = l; }
    void setQuiet(bool q) { quiet_ = q; }
    bool openFile(const std::string& path);
    void log(LogLevel l, const char* fmt, ...) __attribute__((format(printf, 3, 4)));
    ~Logger();
private:
    Logger() = default;
    std::mutex mu_;
    std::FILE* file_ = nullptr;
    LogLevel level_ = LogLevel::Info;
    bool quiet_ = false;
};

}  // namespace sg

#define SG_DEBUG(...) ::sg::Logger::instance().log(::sg::LogLevel::Debug, __VA_ARGS__)
#define SG_INFO(...)  ::sg::Logger::instance().log(::sg::LogLevel::Info,  __VA_ARGS__)
#define SG_WARN(...)  ::sg::Logger::instance().log(::sg::LogLevel::Warn,  __VA_ARGS__)
#define SG_ERROR(...) ::sg::Logger::instance().log(::sg::LogLevel::Error, __VA_ARGS__)
