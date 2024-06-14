#include "stdafx.h"

#include "lv_logger.h"

#include "spdlog/spdlog.h"
#include <spdlog/async.h>

#include "spdlog/sinks/daily_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"


std::string lv_logger::current_time()
{
	time_t now = time(nullptr);

	struct tm tstruct;
	char buf[40];
	tstruct = *localtime(&now);  // NOLINT(concurrency-mt-unsafe)

	//format: HH:mm:ss
	auto ret = strftime(buf, sizeof(buf), "%X", &tstruct);

	if (ret > 0)
		return buf;

	return "xx:xx:xx";
}

void lv_logger::setup()
{
	char tcPathStr[MAX_PATH];
	std::string tsPathStr;

	if (tsPathStr.length() <= 0)
	{
		if (::GetTempPathA(MAX_PATH, tcPathStr) == 0)
		{
			tsPathStr = "C:\\TEMP\\";
		}
		else
		{
			tsPathStr = tcPathStr;
		}
	}

	std::filesystem::path basePath = tsPathStr;
	std::filesystem::path fileName = "lvJanus";
	std::filesystem::path fullPath = basePath / fileName;

	auto fp = std::filesystem::path(fullPath);

	const std::string fn = fp.filename().string();
	const std::string fn_p = fp.remove_filename().string();

	std::string time = current_time();
	std::ranges::replace(time, ':', '_');

	const auto file_log = fn_p + '\\' + fn + "_" + time + ".log";

	
	spdlog::init_thread_pool(8192, 1);

	std::vector<spdlog::sink_ptr> sinks;

	sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
	sinks.push_back(std::make_shared<spdlog::sinks::daily_file_sink_mt>(file_log, 23, 59));

	logger = std::make_shared<spdlog::logger>("lvJ", begin(sinks), end(sinks));
	
	//register to access it globally
	spdlog::set_default_logger(logger);
	spdlog::flush_every(std::chrono::seconds(3));
	spdlog::set_level(lv_logger::log_level);

	spdlog::info("log started - {}", file_log);
}

lv_logger::lv_logger()
{
	setup();
}

lv_logger::~lv_logger()
{
	spdlog::info("log ended");
	spdlog::drop("lvJ");
	
}
