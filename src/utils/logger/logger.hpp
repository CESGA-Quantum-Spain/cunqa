#pragma once

#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_DEBUG

#include <memory>
#include <spdlog/spdlog.h>
#include <spdlog/logger.h>

extern std::shared_ptr<spdlog::logger> logger;

#define LOGGER_TRACE(...) SPDLOG_LOGGER_TRACE(logger, __VA_ARGS__)
#define LOGGER_DEBUG(...) SPDLOG_LOGGER_DEBUG(logger, __VA_ARGS__)
#define LOGGER_INFO(...) SPDLOG_LOGGER_INFO(logger, __VA_ARGS__)
#define LOGGER_WARN(...) SPDLOG_LOGGER_WARN(logger, __VA_ARGS__)
#define LOGGER_ERROR(...) SPDLOG_LOGGER_ERROR(logger, __VA_ARGS__)
#define LOGGER_CRITICAL(...) SPDLOG_LOGGER_CRITICAL(logger, __VA_ARGS__)
 