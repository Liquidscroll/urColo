#pragma once
#include <fstream>
#include <print>
#include <string>
#include <string_view>
#include <unordered_map>

namespace uc {
class Logger {
  public:
    enum class Level { Info, Warn, Error, Ok };

    Logger() = default;

    void setFileLogging(const std::string &filepath) {
        fileStream.open(filepath, std::ios::out | std::ios::app);
        logToFile = fileStream.is_open();
    }

    static void log(Level level, std::string_view message) {
        auto [label, colour] = levelAttributes.at(level);
        std::print("{}{}\033[0m: {}\n", colour, label, message);

        if (logToFile) {
            fileStream << label << ": " << message << '\n';
        }
    }

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
            {Level::Info, {" [INFO]", "\033[36m"}},
            {Level::Warn, {" [WARN]", "\033[33m"}},
            {Level::Error, {"[ERROR]", "\033[31m"}},
            {Level::Ok, {" [INFO]", "\033[32m"}},
    };
};
} // namespace uc
