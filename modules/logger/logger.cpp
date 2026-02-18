#include "logger/logger.h"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <memory>
#include <mutex>

namespace memory_tracer {
namespace logger {

class Logger::Impl {
public:
    Impl() {
        console_sink_ = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink_->set_level(spdlog::level::debug);
        console_sink_->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [thread %t] %v");

        std::vector<spdlog::sink_ptr> sinks = {console_sink_};
        logger_ = std::make_shared<spdlog::logger>("memory_tracer", sinks.begin(), sinks.end());
        logger_->set_level(spdlog::level::debug);
        logger_->flush_on(spdlog::level::err);

        spdlog::set_default_logger(logger_);
    }

    void SetLogLevel(LogLevel level) {
        spdlog::level::level_enum spdlog_level = MapLogLevel(level);
        logger_->set_level(spdlog_level);
        if (console_sink_) {
            console_sink_->set_level(spdlog_level);
        }
        if (file_sink_) {
            file_sink_->set_level(spdlog_level);
        }
    }

    void SetLogFile(const std::string& filepath) {
        std::lock_guard<std::mutex> lock(mutex_);
        try {
            file_sink_ = std::make_shared<spdlog::sinks::basic_file_sink_mt>(filepath, true);
            file_sink_->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [thread %t] %v");
            logger_->sinks().push_back(file_sink_);
        } catch (const std::exception& e) {
            logger_->error("Failed to set log file {}: {}", filepath, e.what());
        }
    }

    void Log(LogLevel level, const std::string& message) {
        spdlog::level::level_enum spdlog_level = MapLogLevel(level);
        logger_->log(spdlog_level, message);
    }

    void Trace(const std::string& message) { logger_->trace(message); }
    void Debug(const std::string& message) { logger_->debug(message); }
    void Info(const std::string& message) { logger_->info(message); }
    void Warn(const std::string& message) { logger_->warn(message); }
    void Error(const std::string& message) { logger_->error(message); }
    void Fatal(const std::string& message) { logger_->critical(message); }

    void Flush() {
        logger_->flush();
    }

private:
    spdlog::level::level_enum MapLogLevel(LogLevel level) {
        switch (level) {
            case LogLevel::TRACE: return spdlog::level::trace;
            case LogLevel::DEBUG: return spdlog::level::debug;
            case LogLevel::INFO:  return spdlog::level::info;
            case LogLevel::WARN:  return spdlog::level::warn;
            case LogLevel::ERROR: return spdlog::level::err;
            case LogLevel::FATAL: return spdlog::level::critical;
            default: return spdlog::level::info;
        }
    }

    std::shared_ptr<spdlog::logger> logger_;
    std::shared_ptr<spdlog::sinks::stdout_color_sink_mt> console_sink_;
    std::shared_ptr<spdlog::sinks::basic_file_sink_mt> file_sink_;
    std::mutex mutex_;
};

Logger::Logger() : pimpl_(std::make_unique<Impl>()) {}
Logger::~Logger() = default;

Logger& Logger::GetInstance() {
    static Logger instance;
    return instance;
}

void Logger::SetLogLevel(LogLevel level) { pimpl_->SetLogLevel(level); }
void Logger::SetLogFile(const std::string& filepath) { pimpl_->SetLogFile(filepath); }
void Logger::Log(LogLevel level, const std::string& message) { pimpl_->Log(level, message); }
void Logger::Trace(const std::string& message) { pimpl_->Trace(message); }
void Logger::Debug(const std::string& message) { pimpl_->Debug(message); }
void Logger::Info(const std::string& message) { pimpl_->Info(message); }
void Logger::Warn(const std::string& message) { pimpl_->Warn(message); }
void Logger::Error(const std::string& message) { pimpl_->Error(message); }
void Logger::Fatal(const std::string& message) { pimpl_->Fatal(message); }
void Logger::Flush() { pimpl_->Flush(); }

} // namespace logger
} // namespace memory_tracer
