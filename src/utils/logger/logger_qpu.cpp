#include <string>
#include "spdlog/sinks/stdout_color_sinks.h"

#include "utils/helpers/environment.hpp"
#include "logger.hpp"

std::shared_ptr<spdlog::logger> logger;

__attribute__((constructor)) void initializeLogger() {
    // QClient logger initialization
    auto id = get_env_variable("SLURM_JOB_ID") + "_" + get_env_variable("SLURM_PROCID");
    auto logger_name = "qpu_logger_" + id;
    logger = spdlog::stdout_color_mt(logger_name);
    logger->set_level(spdlog::level::debug);
    logger->set_pattern("(%D %r) [QPU " + id + "] %^%l: %v %$ %oms");
}