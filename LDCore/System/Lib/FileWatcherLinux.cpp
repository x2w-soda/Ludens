#include <Ludens/Header/Platform.h>
#ifdef LD_PLATFORM_LINUX
#include <Ludens/Header/Hash.h>
#include <Ludens/Log/Log.h>
#include <Ludens/System/FileWatcher.h>
#include <Ludens/System/Memory.h>
#include <filesystem>
#include <string>
#include <sys/inotify.h> // hide
#include <unistd.h>      // hide
#include <unordered_set>
#include <vector>

#define INOTIFY_EVENT_BUF_SIZE 4096

namespace fs = std::filesystem;

namespace LD {

static Log sLog("FileWatcher");

/// we can use inotify to watch both files and directories,
/// current implementation watches on a per-file basis.
struct InotifyWatcher
{
    int handle;           /// inotify watch descritptor handle
    Hash64 hash;          /// canonical file path hash
    std::string filePath; /// canonical file path
};

struct FileWatcherEntry
{
    Hash64 hash;
    std::string filePath;
    FileWatcher::on_modify_callback callback;
    void* user;
};

struct FileWatcherObj
{
    int handle; // inotify file handle
    std::vector<InotifyWatcher> watchers;
    std::vector<FileWatcherEntry> entries;

    FileWatcherObj();
    ~FileWatcherObj();
};

FileWatcherObj::FileWatcherObj()
{
    handle = inotify_init1(IN_NONBLOCK);

    if (handle < 0)
    {
        sLog.error("inotify_init1 failed");
        return;
    }
}

FileWatcherObj::~FileWatcherObj()
{
    close(handle);
}

FileWatcher FileWatcher::create()
{
    FileWatcherObj* obj = heap_new<FileWatcherObj>(MEMORY_USAGE_MISC);

    return {obj};
}

void FileWatcher::destroy(FileWatcher watcher)
{
    FileWatcherObj* obj = watcher;

    heap_delete<FileWatcherObj>(obj);
}

void FileWatcher::add_file(const FS::Path& path, on_modify_callback callback, void* user)
{
    std::string canon = fs::canonical(path).string();
    Hash64 canonHash(canon.c_str());

    bool hasWatcher = false;
    for (const InotifyWatcher& watcher : mObj->watchers)
    {
        if (watcher.hash == canonHash)
        {
            hasWatcher = true;
            break;
        }
    }

    if (!hasWatcher)
    {
        InotifyWatcher watcher;
        watcher.handle = inotify_add_watch(mObj->handle, canon.c_str(), IN_CLOSE_WRITE);
        watcher.filePath = canon.string();
        watcher.hash = canonHash;

        if (watcher.handle < 0)
        {
            sLog.error("inotify_add_watch failed");
            return;
        }

        mObj->watchers.push_back(watcher);
    }

    FileWatcherEntry entry{
        .hash = canonHash,
        .filePath = path.string(),
        .callback = callback,
        .user = user,
    };
    mObj->entries.push_back(entry);
    sLog.info("add_file    {}", path.string());
}

void FileWatcher::remove_file(const FS::Path& path)
{
    // TODO:
}

int FileWatcher::has_file(const FS::Path& path)
{
    std::string canon = fs::canonical(path).string();
    Hash64 canonHash(canon.c_str());

    int count = 0;

    for (const FileWatcherEntry& entry : mObj->entries)
    {
        if (entry.hash == canonHash)
            count++;
    }

    return count;
}

void FileWatcher::poll()
{
    alignas(struct inotify_event) char buf[INOTIFY_EVENT_BUF_SIZE];
    struct inotify_event* event = nullptr;

    // just in case there are repeated events on the same file during a single poll.
    std::unordered_set<Hash64> fileHashes;

    while (true)
    {
        ssize_t ret = read(mObj->handle, buf, sizeof(buf));

        if (ret <= 0)
            break;

        for (ssize_t i = 0; i < ret; i += event->len + sizeof(struct inotify_event))
        {
            event = (struct inotify_event*)(buf + i);

            InotifyWatcher* watch = nullptr;

            for (size_t i = 0; i < mObj->watchers.size(); i++)
            {
                if (mObj->watchers[i].handle == event->wd)
                {
                    watch = mObj->watchers.data() + i;
                    break;
                }
            };

            if (!watch)
            {
                sLog.error("inotify watch missing!?");
                return;
            }

            if (event->mask & IN_IGNORED)
            {
                // File is ignored, attach a new watcher.
                // Text editors such as vim that have a swap-file mechanism may trigger this event.
                watch->handle = inotify_add_watch(mObj->handle, watch->filePath.c_str(), IN_CLOSE_WRITE);

                if (watch->handle <= 0)
                {
                    sLog.info("inotify_add_watch failed");
                    return;
                }
            }

            if (event->mask & (IN_CLOSE_WRITE | IN_IGNORED))
            {
                fileHashes.insert(watch->hash);
            }
        }
    }

    // trigger user callbacks
    for (FileWatcherEntry& entry : mObj->entries)
    {
        if (fileHashes.contains(entry.hash))
            entry.callback(entry.filePath.c_str(), entry.user);
    }
}

} // namespace LD
#endif // LD_PLATFORM_LINUX
