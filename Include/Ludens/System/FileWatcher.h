#pragma once

#include <Ludens/Header/Handle.h>

namespace LD {

struct FileWatcher : Handle<struct FileWatcherObj>
{
    typedef void (*on_modify_callback)(const char* path, void* user);

    /// @brief create a file watcher
    /// @return a new file watcher handle
    static FileWatcher create();

    /// @brief destroy a file watcher
    /// @param watcher a file watcher handle
    static void destroy(FileWatcher watcher);

    /// @brief add a file to watch, multiple callbacks can be added to the same file.
    /// @param path file path to watch
    /// @param callback user callback when the file is modified
    /// @param user arbitrary user data
    void add_file(const char* path, on_modify_callback callback, void* user);

    /// @brief remove all watcher callbacks associated with file
    /// @param path file path currently being watched
    void remove_file(const char* path);

    /// @brief checks if a file is already being watched
    /// @param path file path to query
    /// @return number of callbacks associated with file path
    int has_file(const char* path);

    /// @brief asynchronous poll to check if files are modified,
    ///        blocks until all user callbacks are completed.
    void poll();
};

} // namespace LD