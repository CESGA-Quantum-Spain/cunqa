#include "logger.hpp"

std::shared_ptr<spdlog::logger> logger;

__attribute__((constructor)) void initializeLogger() {
    // QClient logger initialization
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_level(spdlog::level::warn);

    std::string store_path = std::getenv("STORE");
    auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(store_path + "/.cunqa/logs/logging.log", 1024*1024, 5, false);
    file_sink->set_level(spdlog::level::debug);

    spdlog::sinks_init_list sinks = { file_sink, console_sink };
    logger = std::make_shared<spdlog::logger>("client_logger", std::begin(sinks), std::end(sinks));
    logger->sinks()[0]->set_pattern("(%D %r) [QClient] %s: %^%l: %v %$ %oms");
    logger->sinks()[1]->set_pattern("%@\n\t%^%l: %v %$ %oms");

    logger->set_level(spdlog::level::debug);
    spdlog::register_logger(logger);
}