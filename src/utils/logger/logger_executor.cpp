#include <string>

#include "spdlog/sinks/stdout_color_sinks.h"

#include "logger.hpp"

std::shared_ptr<spdlog::logger> logger;

__attribute__((constructor)) void initializeLogger() {
    std::string name = "executor_logger";
    logger = spdlog::stdout_color_mt(name);
    logger->set_level(spdlog::level::debug);
    logger->set_pattern("(%D %r) [Executor] %^%l: %v %$ %oms");
}