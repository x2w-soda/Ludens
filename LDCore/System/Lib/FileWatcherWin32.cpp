#include <Ludens/Header/Platform.h>
#ifdef LD_PLATFORM_WIN32
#include <Ludens/Header/Hash.h>
#include <Ludens/Log/Log.h>
#include <Ludens/System/FileWatcher.h>
#include <Ludens/System/Memory.h>
#include <Windows.h> // hide
#include <algorithm>
#include <array>
#include <filesystem>
#include <string>
#include <unordered_set>
#include <vector>

namespace fs = std::filesystem;

#define DIRECTORY_BUFFER_SIZE (4096 * 4)

namespace LD {

static Log sLog("FileWatcher");

struct FileWatcherObj;
struct FileWatcherEntry;

struct Win32Directory
{
    OVERLAPPED overlapped;
    HANDLE handle;
    DWORD notifyFilter;
    Hash64 hash;
    FileWatcherObj* watcher;
    std::array<BYTE, DIRECTORY_BUFFER_SIZE> buffer;

    Win32Directory() = delete;
    Win32Directory(Hash64 hash, const char* dirPath, FileWatcherObj* watcher);
    Win32Directory(const Win32Directory&) = delete;
    ~Win32Directory();

    Win32Directory& operator=(const Win32Directory&) = delete;
};

Win32Directory::Win32Directory(Hash64 hash, const char* dirPath, FileWatcherObj* watcher)
    : overlapped{}, hash(hash), watcher(watcher), buffer{}
{
    notifyFilter = FILE_NOTIFY_CHANGE_LAST_WRITE;

    DWORD shareMode = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
    DWORD creationDisposition = OPEN_EXISTING;
    DWORD flagsAndAttrs = FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED;
    handle = CreateFile(dirPath, FILE_LIST_DIRECTORY, shareMode, NULL, creationDisposition, flagsAndAttrs, NULL);

    if (handle != INVALID_HANDLE_VALUE)
    {
        overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    }
    else
    {
        sLog.warn("failed to open Win32 directory {}", dirPath);
    }
}

Win32Directory::~Win32Directory()
{
    if (overlapped.hEvent)
        CloseHandle(overlapped.hEvent);

    if (handle)
        CloseHandle(handle);
}

static void win32_refresh_watch(Win32Directory* dir);
static void CALLBACK win32_watch_callback(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped);

struct FileWatcherEntry
{
    Hash64 dirHash;
    Hash64 fileHash;
    std::string filePath;
    FileWatcher::on_modify_callback callback;
    void* user;
};

/// @brief Win32 implementation of file watcher
struct FileWatcherObj
{
    std::vector<FileWatcherEntry> entries;
    std::vector<Win32Directory*> watchedDirs;
    std::unordered_set<Hash64> polledFiles;

    size_t get_watched_dir_index(Hash64 hash);
};

size_t FileWatcherObj::get_watched_dir_index(Hash64 hash)
{
    size_t idx = 0;

    for (; idx < watchedDirs.size(); idx++)
    {
        if (hash == watchedDirs[idx]->hash)
            return idx;
    }

    return idx;
}

static void win32_refresh_watch(Win32Directory* dir)
{
    ReadDirectoryChangesW(dir->handle, dir->buffer.data(), (DWORD)dir->buffer.size(), false, dir->notifyFilter, NULL, &dir->overlapped, &win32_watch_callback);
}

// LPOVERLAPPED_COMPLETION_ROUTINE
static void CALLBACK win32_watch_callback(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped)
{
    if (dwNumberOfBytesTransfered == 0)
        return;

    if (dwErrorCode != ERROR_SUCCESS)
        return;

    char callbackFileName[MAX_PATH];
    PFILE_NOTIFY_INFORMATION pNotify;
    Win32Directory* dir = (Win32Directory*)lpOverlapped;
    FileWatcherObj* watcher = dir->watcher;
    size_t offset = 0;

    do
    {
        pNotify = (PFILE_NOTIFY_INFORMATION)&dir->buffer[offset];
        offset += pNotify->NextEntryOffset;

        if (pNotify->Action != FILE_ACTION_MODIFIED)
            continue;

        int nameSize = WideCharToMultiByte(CP_ACP, 0, pNotify->FileName,
                                           pNotify->FileNameLength / sizeof(WCHAR),
                                           callbackFileName, MAX_PATH - 1, NULL, NULL);

        watcher->polledFiles.insert(Hash64(callbackFileName, nameSize));
    } while (pNotify->NextEntryOffset != 0);

    win32_refresh_watch(dir);
}

FileWatcher FileWatcher::create()
{
    FileWatcherObj* obj = heap_new<FileWatcherObj>(MEMORY_USAGE_MISC);

    return {obj};
}

void FileWatcher::destroy(FileWatcher watcher)
{
    FileWatcherObj* obj = watcher;

    for (Win32Directory* dir : obj->watchedDirs)
        heap_delete<Win32Directory>(dir);

    heap_delete<FileWatcherObj>(obj);
}

void FileWatcher::add_file(const FS::Path& path, on_modify_callback callback, void* user)
{
    fs::path canon = fs::canonical(path);
    std::string canonFile = canon.filename().string();
    std::string canonDir = fs::path(canon).remove_filename().string();
    Hash64 canonFileHash(canonFile.c_str());
    Hash64 canonDirHash(canonDir.c_str());

    size_t idx = mObj->get_watched_dir_index(canonDirHash);

    // new win32 directory to monitor
    if (idx == mObj->watchedDirs.size())
    {
        Win32Directory* dir = heap_new<Win32Directory>(MEMORY_USAGE_MISC, canonDirHash, canonDir.c_str(), mObj);

        mObj->watchedDirs.push_back(dir);

        win32_refresh_watch(dir);
    }

    FileWatcherEntry entry{
        .dirHash = canonDirHash,
        .fileHash = canonFileHash,
        .filePath = path.string(),
        .callback = callback,
        .user = user,
    };
    mObj->entries.push_back(entry);

    sLog.info("add_file {}", path.string());
}

void FileWatcher::remove_file(const FS::Path& path)
{
    fs::path canon = fs::canonical(path);
    std::string canonFile = canon.filename().string();
    std::string canonDir = fs::path(canon).remove_filename().string();
    Hash64 fileHash(canonFile.c_str());
    Hash64 dirHash(canonDir.c_str());

    size_t count = std::erase_if(mObj->entries, [&](const FileWatcherEntry& entry) -> bool {
        return entry.dirHash == dirHash && entry.fileHash == fileHash;
    });

    if (count > 0)
        sLog.info("remove_file {}", path.string());

    // directories to keep
    std::unordered_set<Hash64> dirHashes;
    for (const FileWatcherEntry& entry : mObj->entries)
        dirHashes.insert(entry.dirHash);

    // we need iterators for unwatched directories, back to erase-remove idiom.
    const auto& unwatched = std::remove_if(mObj->watchedDirs.begin(), mObj->watchedDirs.end(), [&](Win32Directory* dir) -> bool {
        return !dirHashes.contains(dir->hash);
    });

    for (auto ite = unwatched; ite != mObj->watchedDirs.end(); ite++)
    {
        heap_delete<Win32Directory>(*ite);
    }

    // we can safely erase vector elements now
    mObj->watchedDirs.erase(unwatched, mObj->watchedDirs.end());
}

int FileWatcher::has_file(const FS::Path& path)
{
    fs::path canon = fs::canonical(path);
    std::string canonFile = canon.filename().string();
    std::string canonDir = fs::path(canon).remove_filename().string();
    Hash64 fileHash(canonFile.c_str());
    Hash64 dirHash(canonDir.c_str());

    int count = 0;

    for (const FileWatcherEntry& entry : mObj->entries)
    {
        if (entry.dirHash == dirHash && entry.fileHash == fileHash)
            count++;
    }

    return count;
}

void FileWatcher::poll()
{
    mObj->polledFiles.clear();

    // Non-blocking, asynchronous polling. It is common for Win32 to
    // call our win32_watch_callback multiple times on the same file,
    // so we use an unordered_set to filter duplicates.
    MsgWaitForMultipleObjectsEx(0, NULL, 0, QS_ALLINPUT, MWMO_ALERTABLE);

    if (mObj->polledFiles.empty())
        return;

    // process user callbacks
    for (const FileWatcherEntry& entry : mObj->entries)
    {
        if (mObj->polledFiles.contains(entry.fileHash))
            entry.callback(entry.filePath.c_str(), entry.user);
    }
}

} // namespace LD
#endif // LD_PLATFORM_WIN32
