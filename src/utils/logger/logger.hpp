#pragma once

#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_DEBUG

#include <iostream>
#include <memory>
#include <vector>
#include <string>
#include <spdlog/spdlog.h>
#include <spdlog/logger.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

using namespace std::literals;

//TODO: This generates a client logger for the QPUs also (improve this)
extern std::shared_ptr<spdlog::logger> logger;