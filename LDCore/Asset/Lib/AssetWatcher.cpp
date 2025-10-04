#include "AssetWatcher.h"
#include <Ludens/Header/Assert.h>

namespace LD {

void AssetWatcher::startup(const AssetWatcherInfo& info)
{
    mWatcher = FileWatcher::create();
    mUserCallback = info.onAssetModified;
    mUser = info.user;
}

void AssetWatcher::cleanup()
{
    FileWatcher::destroy(mWatcher);
    mPathToID.clear();
    mUserCallback = nullptr;
    mUser = nullptr;
}

void AssetWatcher::add_watch(const FS::Path& path, AUID auid)
{
    Hash32 hash(path.string().c_str());

    if (mPathToID.contains(hash))
        return;

    mPathToID[hash] = auid;

    mWatcher.add_file(path, &AssetWatcher::on_file_modify, this);
}

void AssetWatcher::on_file_modify(const FS::Path& path, void* user)
{
    AssetWatcher& self = *(AssetWatcher*)user;

    Hash32 hash(path.string().c_str());
    LD_ASSERT(self.mPathToID.contains(hash));
    AUID assetID = self.mPathToID[hash];

    if (self.mUserCallback)
        self.mUserCallback(path, assetID, self.mUser);
}

} // namespace LD