#pragma once

#include <Ludens/Asset/Asset.h>
#include <Ludens/Header/Hash.h>
#include <Ludens/System/FileWatcher.h>
#include <unordered_map>

namespace LD {

struct AssetWatcherInfo
{
    void (*onAssetModified)(const FS::Path& path, AUID id, void* user);
    void* user;
};

/// @brief Asset file watcher.
class AssetWatcher
{
public:
    /// @brief In-place startup
    void startup(const AssetWatcherInfo& info);

    /// @brief In-place cleanup
    void cleanup();

    /// @brief Poll for asset file changes.
    inline void poll()
    {
        if (!mWatcher)
            return;

        mWatcher.poll();
    }

    /// @brief Watch an asset path.
    /// @param path File path to asset on disk.
    /// @param auid Associated asset ID.
    void add_watch(const FS::Path& path, AUID auid);

    inline operator bool() const { return (bool)mWatcher; }

    static void on_file_modify(const FS::Path& path, void* user);

private:
    FileWatcher mWatcher = {};
    std::unordered_map<Hash32, AUID> mPathToID;
    void (*mUserCallback)(const FS::Path& path, AUID id, void* user);
    void* mUser;
};

} // namespace LD