#pragma once

#include <Ludens/DSA/HashMap.h>
#include <Ludens/DSA/Vector.h>
#include <Ludens/Serial/SUID.h>

namespace LD {

struct SUIDEntry
{
    SUID id;
    std::string path;
};

/// @brief Table to lookup between URIs (path or name) and Serial IDs.
class SUIDTable
{
public:
    inline bool contains(SUID id) const { return mIDs.contains(id); }
    SUID find_by_path(const std::string& path);
    SUID find_by_name(const std::string& name);
    bool register_id(SUID id, const std::string& path);
    void unregister_id(SUID id);
    bool is_path_valid(const std::string& path, std::string& collidingPath);
    bool is_path_rename_valid(SUID id, const std::string& newPath);
    bool set_path(SUID id, const std::string& newPath);
    bool get_path(SUID id, std::string& outPath);
    bool get_name(SUID id, std::string& outName);

    Vector<SUIDEntry> get_entries();

private:
    HashMap<SUID, std::string> mIDs;   // ID to path
    HashMap<std::string, SUID> mPaths; // path to ID
    HashMap<std::string, SUID> mNames; // name to ID
};

} // namespace LD