#include "logger.hpp"

std::shared_ptr<spdlog::logger> logger;

__attribute__((constructor)) void initializeLogger() {
    // QClient logger initialization
    std::string id = std::getenv("SLURM_JOB_ID") + "_"s + std::getenv("SLURM_TASK_PID"); //Alvaro: antes SLURM_LOCALID
    std::string qpu_name = "qpu_logger_"s + id;
    logger = spdlog::stdout_color_mt(qpu_name);
    logger->set_level(spdlog::level::debug);
    logger->set_pattern("(%D %r) [QPU "s + id + "] %^%l: %v %$ %oms"s);
}