#pragma once

#include <Ludens/Header/Handle.h>
#include <format>
#include <string>
#include <utility>

namespace LD {

enum LogLevel
{
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR,
};

// forward declarations
struct LogObj;
void log_message(LogObj* obj, LogLevel level, const std::string& msg);

struct Log : Handle<LogObj>
{
    /// @brief get logger handle for the default channel
    Log();

    /// @brief get logger handle for channel name
    /// @param channelName channel name c string
    Log(const char* channelName);

    template <typename... TArgs>
    void debug(const std::format_string<TArgs...>& fmt, TArgs&&... args)
    {
        std::string msg = std::vformat(fmt.get(), std::make_format_args(args...));
        log_message(mObj, LOG_LEVEL_DEBUG, msg);
    }

    template <typename... TArgs>
    void info(const std::format_string<TArgs...>& fmt, TArgs&&... args)
    {
        std::string msg = std::vformat(fmt.get(), std::make_format_args(args...));
        log_message(mObj, LOG_LEVEL_INFO, msg);
    }

    template <typename... TArgs>
    void warn(const std::format_string<TArgs...>& fmt, TArgs&&... args)
    {
        std::string msg = std::vformat(fmt.get(), std::make_format_args(args...));
        log_message(mObj, LOG_LEVEL_WARN, msg);
    }

    template <typename... TArgs>
    void error(const std::format_string<TArgs...>& fmt, TArgs&&... args)
    {
        std::string msg = std::vformat(fmt.get(), std::make_format_args(args...));
        log_message(mObj, LOG_LEVEL_ERROR, msg);
    }
};

} // namespace LD