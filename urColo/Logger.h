// urColo - simple logging helper
#pragma once
#include <format>
#include <fstream>
#include <print>
#include <string>
#include <string_view>
#include <unordered_map>

#define _INFO(x) Logger::log(Logger::Level::Info, x)
#define _WARN(x) Logger::log(Logger::Level::Warn, x)
#define _ERROR(x) Logger::log(Logger::Level::Error, x)
#define _OK(x) Logger::log(Logger::Level::Ok, x)
namespace uc {
// Logger outputs colour-coded messages to stdout and can also
// duplicate them into an optional log file.
class Logger {
  public:
    // Severity level for a log entry. 'Info' and 'Ok' denote normal
    // informational messages, 'Warn' indicates a potential issue and
    // 'Error' reports a problem.
    enum class Level { Info, Warn, Error, Ok };

    // Construct a logger. By default only stdout logging is active.
    Logger() = default;

    // Enable logging to the given file path. If the file cannot be
    // opened, logging continues only to stdout.
    void setFileLogging(const std::string &filepath) {
        fileStream.open(filepath, std::ios::out | std::ios::app);
        logToFile = fileStream.is_open();
    }
    // Print a formatted message with the requested severity. The output is
    // sent to stdout and duplicated to the log file if enabled. Supports both
    // plain strings and format strings with arguments.
    template <typename... Args>
    static void log(Level level, std::string_view fmt, Args &&...args) {
        auto [label, colour] = levelAttributes.at(level);
        std::string message = std::vformat(fmt, std::make_format_args(args...));
        std::print("[{}{}\033[0m]: {}\n", colour, label, message);

        if (logToFile) {
            fileStream << label << ": " << message << '\n';
        }
    }

    // Close the file stream if file logging is active. Safe to call
    // multiple times.
    void shutdown() {
        if (fileStream.is_open()) {
            fileStream.close();
        }
    }

  private:
    inline static bool logToFile = false;
    inline static std::ofstream fileStream;

    inline static const std::unordered_map<
        Level, std::pair<std::string_view, std::string_view>>
        levelAttributes = {
            {Level::Info, {" INFO", "\033[36m"}},
            {Level::Warn, {" WARN", "\033[33m"}},
            {Level::Error, {"ERROR", "\033[31m"}},
            {Level::Ok, {" INFO", "\033[32m"}},
    };
};
} // namespace uc
