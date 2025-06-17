#include "urColo/Logger.h"
#include <doctest/doctest.h>
#include <filesystem>
#include <fstream>

TEST_CASE("logger writes labelled entries") {
    uc::Logger logger;
    std::filesystem::path path =
        std::filesystem::temp_directory_path() / "urcolo_logger_test.log";
    std::filesystem::remove(path);
    logger.setFileLogging(path.string());

    uc::Logger::log(uc::Logger::Level::Info, "info message");
    uc::Logger::log(uc::Logger::Level::Warn, "warn message");
    uc::Logger::log(uc::Logger::Level::Error, "error message");
    uc::Logger::log(uc::Logger::Level::Ok, "ok message");
    logger.shutdown();
    logger.shutdown();

    std::ifstream file(path);
    REQUIRE(file.is_open());
    std::string l1, l2, l3, l4;
    std::getline(file, l1);
    std::getline(file, l2);
    std::getline(file, l3);
    std::getline(file, l4);
    file.close();

    CHECK(l1 == " INFO: info message");
    CHECK(l2 == " WARN: warn message");
    CHECK(l3 == "ERROR: error message");
    CHECK(l4 == " INFO: ok message");

    std::filesystem::remove(path);
}
