#pragma once

#include <iostream>
#include <utility>

#include "debug.h"

namespace kettle
{
enum class LogLevel
{
    Info,
    Warning,
    Error,
};

inline const char* toString(LogLevel level)
{
    switch (level)
    {
    case LogLevel::Info:
        return "info";
    case LogLevel::Warning:
        return "warning";
    case LogLevel::Error:
        return "error";
    }

    return "unknown";
}

template<typename... Args>
void log(LogLevel level, Args&&... args)
{
#if KETTLE_DEBUG
    std::cerr << "[kettle][" << toString(level) << "] ";
    (std::cerr << ... << std::forward<Args>(args));
    std::cerr << "\n";
#else
    (void)level;
#endif
}
} // namespace kettle

#if KETTLE_DEBUG
#define KETTLE_LOG_INFO(...) ::kettle::log(::kettle::LogLevel::Info, __VA_ARGS__)
#define KETTLE_LOG_WARNING(...) ::kettle::log(::kettle::LogLevel::Warning, __VA_ARGS__)
#define KETTLE_LOG_ERROR(...) ::kettle::log(::kettle::LogLevel::Error, __VA_ARGS__)
#else
#define KETTLE_LOG_INFO(...) ((void)0)
#define KETTLE_LOG_WARNING(...) ((void)0)
#define KETTLE_LOG_ERROR(...) ((void)0)
#endif