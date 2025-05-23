#include <Ludens/DSA/Hash.h>
#include <Ludens/Log/Log.h>
#include <Ludens/System/Memory.h>
#include <format>
#include <iostream>
#include <unordered_map>

namespace LD {

/// @brief logger object for a single channel
struct LogObj
{
    std::string name;
};

class LogChannels
{
public:
    /// @brief get singleton
    static LogChannels* get();

    /// @brief get or create logger for channel
    static LogObj* get_log(const char* channelName);

private:
    LogObj mDefault;
    std::unordered_map<uint32_t, LogObj*> mChannels;
    static LogChannels* sInstance;
};

LogChannels* LogChannels::sInstance = nullptr;
static const char* get_log_level_name(LogLevel level);

LogChannels* LogChannels::get()
{
    if (!sInstance)
    {
        // NOTE: currently all loggers can't be destroyed and
        //       will live until the very end of program.
        sInstance = new LogChannels();
    }

    return sInstance;
}

LogObj* LogChannels::get_log(const char* channelName)
{
    LogChannels* self = LogChannels::get();

    if (!channelName)
        return &self->mDefault;

    uint32_t hash32 = hash32_FNV_1a(channelName, strlen(channelName));
    if (self->mChannels.contains(hash32))
        return self->mChannels[hash32];

    LogObj* obj = new LogObj();
    obj->name = channelName;

    self->mChannels[hash32] = obj;

    return obj;
}

const char* get_log_level_name(LogLevel level)
{
    switch (level)
    {
    case LOG_LEVEL_DEBUG:
        return "[DEBUG]";
    case LOG_LEVEL_INFO:
        return "[INFO]";
    case LOG_LEVEL_WARN:
        return "[WARN]";
    case LOG_LEVEL_ERROR:
        return "[ERROR]";
    }

    return nullptr;
}

// TODO: this needs to be thread safe, same logger may be accessed from multiple threads
void log_message(LogObj* obj, LogLevel level, const std::string& msg)
{
    std::string prefix(get_log_level_name(level));

    bool isDefaultChannel = obj == LogChannels::get_log(nullptr);
    if (!isDefaultChannel)
    {
        prefix.push_back('[');
        prefix += obj->name;
        prefix.push_back(']');
    }

    std::cout << prefix << ' ' << msg << std::endl;
}

Log::Log()
{
    mObj = LogChannels::get_log(nullptr);
}

Log::Log(const char* channelName)
{
    mObj = LogChannels::get_log(channelName);
}

} // namespace LD