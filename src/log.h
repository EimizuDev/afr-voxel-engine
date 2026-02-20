#pragma once

#include <spdlog/spdlog.h>

#define AFRE_INFO(msg) spdlog::info(msg)
#define AFRE_WARN(msg) spdlog::warn(msg)
#define AFRE_ERROR(msg) spdlog::error(msg)
#define AFRE_CRIT(msg, returnVal) spdlog::critical(msg)