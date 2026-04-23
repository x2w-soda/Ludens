#include <Ludens/Header/Assert.h>
#include <Ludens/Serial/SUIDTable.h>
#include <Ludens/System/FileSystem.h>

namespace LD {

SUID SUIDTable::find_by_path(const std::string& path)
{
    const auto& it = mPaths.find(path);

    if (it != mPaths.end())
        return it->second;

    return SUID(0);
}

SUID SUIDTable::find_by_name(const std::string& name)
{
    const auto& it = mNames.find(name);

    if (it != mNames.end())
        return it->second;

    return SUID(0);
}

bool SUIDTable::register_id(SUID id, const std::string& path)
{
    std::string colliding;
    if (!id || mIDs.contains(id) || !is_path_valid(path, colliding))
        return false;

    mIDs[id] = path;
    mPaths[path] = id;

    std::string name = FS::Path(path).stem().string();
    mNames[name] = id;
    return true;
}

void SUIDTable::unregister_id(SUID id)
{
    const auto& it = mIDs.find(id);
    if (it == mIDs.end())
        return;

    const std::string& path = it->second;
    std::string name = FS::Path(it->second).stem().string();

    mPaths.erase(path);
    mNames.erase(name);
    mIDs.erase(id);
}

bool SUIDTable::is_path_valid(const std::string& path, std::string& collidingPath)
{
    std::string name = FS::Path(path).stem().string();

    if (mPaths.contains(path))
    {
        collidingPath = path;
        return false;
    }

    if (mNames.contains(name))
    {
        SUID collidingID = mNames[name];
        LD_ASSERT(mIDs.contains(collidingID));

        collidingPath = mIDs[collidingID];
        return false;
    }

    return true;
}

bool SUIDTable::is_path_rename_valid(SUID id, const std::string& newPath)
{
    const auto& it = mIDs.find(id);
    if (it == mIDs.end())
        return false;

    const std::string& oldPath = it->second;
    std::string oldName = FS::Path(oldPath).stem().string();
    std::string newName = FS::Path(newPath).stem().string();

    if (newPath == oldPath)
        return true; // this could also be false depending on semantics...

    if (mPaths.contains(newPath))
        return false;

    // if we change stem name, it must not collide with existing names.
    if (newName != oldName && mNames.contains(newName))
        return false;

    return true;
}

bool SUIDTable::set_path(SUID id, const std::string& newPath)
{
    if (!is_path_rename_valid(id, newPath))
        return false;

    LD_ASSERT(mIDs.contains(id));
    std::string oldPath = mIDs[id];
    std::string oldName = FS::Path(oldPath).stem().string();

    LD_ASSERT(mPaths.contains(oldPath));
    mPaths.erase(oldPath);

    LD_ASSERT(mNames.contains(oldName));
    mNames.erase(oldName);

    std::string newName = FS::Path(newPath).stem().string();

    mIDs[id] = newPath;
    mPaths[newPath] = id;
    mNames[newName] = id;

    return true;
}

bool SUIDTable::get_path(SUID id, std::string& outPath)
{
    const auto it = mIDs.find(id);

    if (it == mIDs.end())
    {
        outPath.clear();
        return false;
    }

    outPath = it->second;
    return true;
}

bool SUIDTable::get_name(SUID id, std::string& outName)
{
    std::string str;
    if (!get_path(id, str))
        return false;

    outName = FS::Path(str).stem().string();
    return true;
}

Vector<SUIDEntry> SUIDTable::get_entries()
{
    Vector<SUIDEntry> entries;

    entries.reserve(mIDs.size());

    for (const auto& it : mIDs)
        entries.emplace_back(it.first, it.second);

    return entries;
}

} // namespace LD