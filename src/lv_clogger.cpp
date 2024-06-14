
#include "stdafx.h"

#include "lv_clogger.h"

#include <spdlog/spdlog.h>

#include "lv_logger.h"

void lv_logger_info(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    // Determine the size of the formatted string
    va_list args_copy;
    va_copy(args_copy, args);
    const int size = std::vsnprintf(nullptr, 0, fmt, args_copy) + 1; // Add 1 for the null terminator
    va_end(args_copy);

    // Create a buffer to hold the formatted string
    std::vector<char> buffer(size);
    std::vsnprintf(buffer.data(), size, fmt, args);

    va_end(args);

    std::string formatted_msg = buffer.data();

    spdlog::info(formatted_msg);
}
