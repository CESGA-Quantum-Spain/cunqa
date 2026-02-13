#include "logger.hpp"
#include <string>
#include <spdlog/sinks/stdout_color_sinks.h>

using namespace std::literals;

std::shared_ptr<spdlog::logger> logger;

__attribute__((constructor)) void initializeLogger() {
    // QClient logger initialization
    auto ids = std::getenv("SLURM_JOB_ID");
    std::string id;
    if (ids != nullptr)
	id = ids;
    std::string qpu_name = "executor_logger_"s + id;
    logger = spdlog::stdout_color_mt(qpu_name);
    logger->set_level(spdlog::level::debug);
    logger->set_pattern("(%D %r) [Executor "s + id + "] %^%l: %v %$ %oms"s);
}