#pragma once

#include <Ludens/DSA/HashMap.h>
#include <Ludens/DSA/String.h>
#include <Ludens/DSA/Vector.h>
#include <Ludens/Serial/SUID.h>

namespace LD {

struct SUIDEntry
{
    SUID id;
    String path;
};

/// @brief Table to lookup between URIs (path or name) and Serial IDs.
class SUIDTable
{
public:
    inline bool contains(SUID id) const { return mIDs.contains(id); }
    SUID find_by_path(const String& path);
    SUID find_by_name(const String& name);
    bool register_id(SUID id, const String& path);
    void unregister_id(SUID id);
    bool is_path_valid(const String& path, String& collidingPath);
    bool is_path_rename_valid(SUID id, const String& newPath);
    bool set_path(SUID id, const String& newPath);
    bool get_path(SUID id, String& outPath);
    bool get_name(SUID id, String& outName);

    Vector<SUIDEntry> get_entries();

private:
    HashMap<SUID, String> mIDs;   // ID to path
    HashMap<String, SUID> mPaths; // path to ID
    HashMap<String, SUID> mNames; // name to ID
};

} // namespace LD