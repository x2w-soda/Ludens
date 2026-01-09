#include <Ludens/DSA/HashMap.h>
#include <Ludens/DSA/HashSet.h>
#include <Ludens/Header/Hash.h>
#include <Ludens/Log/Log.h>
#include <Ludens/Memory/Memory.h>

#include <cstring>
#include <format>
#include <iostream>
#include <mutex>

namespace LD {

/// @brief Logger object implementation.
struct LogObj
{
    LogObj() = default;

    LogObj(const char* channelName)
        : name(channelName)
    {
    }

    const std::string name;
    std::mutex mtx;
    std::unordered_set<LogObserver> observers;
};

class LogChannels
{
public:
    /// @brief Get singleton.
    static LogChannels* get();

    /// @brief Get or create logger for channel.
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

    // NOTE: heap_new from LDSystem tracks allocations and reports memory leaks,
    //       here we are using vanilla 'new' since per-channel logs live
    //       until the very end of application lifetime.
    LogObj* obj = new LogObj(channelName);

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

void log_message(LogObj* obj, LogLevel level, const std::string& msg)
{
    std::string prefix(get_log_level_name(level));

    bool isDefaultChannel = obj == LogChannels::get_log(nullptr);
    if (!isDefaultChannel)
    {
        prefix.push_back('[');
        prefix += obj->name; // channel name is RO
        prefix.push_back(']');
    }

    // This introduces per-message mutex contention...
    // Refactor later if this is an observable bottleneck in profiling.
    {
        std::unique_lock<std::mutex> lock(obj->mtx);
        for (LogObserver observer : obj->observers)
            observer(level, msg);
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

void Log::add_observer(LogObserver observer)
{
    std::unique_lock<std::mutex> lock(mObj->mtx);

    mObj->observers.insert(observer);
}

void Log::remove_observer(LogObserver observer)
{
    std::unique_lock<std::mutex> lock(mObj->mtx);

    mObj->observers.erase(observer);
}

} // namespace LD
