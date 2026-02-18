#pragma once

#include <string>
#include <memory>

namespace memory_tracer {
namespace logger {

enum class LogLevel {
    TRACE,
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL
};

class Logger {
public:
    static Logger& GetInstance();

    void SetLogLevel(LogLevel level);
    void SetLogFile(const std::string& filepath);

    void Log(LogLevel level, const std::string& message);
    void Trace(const std::string& message);
    void Debug(const std::string& message);
    void Info(const std::string& message);
    void Warn(const std::string& message);
    void Error(const std::string& message);
    void Fatal(const std::string& message);

    void Flush();

private:
    Logger();
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

#define LOG_TRACE(msg) memory_tracer::logger::Logger::GetInstance().Trace(msg)
#define LOG_DEBUG(msg) memory_tracer::logger::Logger::GetInstance().Debug(msg)
#define LOG_INFO(msg)  memory_tracer::logger::Logger::GetInstance().Info(msg)
#define LOG_WARN(msg)  memory_tracer::logger::Logger::GetInstance().Warn(msg)
#define LOG_ERROR(msg) memory_tracer::logger::Logger::GetInstance().Error(msg)
#define LOG_FATAL(msg) memory_tracer::logger::Logger::GetInstance().Fatal(msg)

} // namespace logger
} // namespace memory_tracer
