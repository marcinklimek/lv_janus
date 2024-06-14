#pragma once

#include <filesystem>
#include <spdlog/logger.h>

class lv_logger
{
	const spdlog::level::level_enum log_level = spdlog::level::trace;
	std::string current_time();

	std::shared_ptr<spdlog::logger> logger;

public:

	void setup();

	lv_logger();
	~lv_logger();
	lv_logger(const lv_logger& other) = delete;
	lv_logger(lv_logger&& other) noexcept = delete;
	lv_logger& operator=(const lv_logger& other) = delete;
	lv_logger& operator=(lv_logger&& other) noexcept = delete;
};

