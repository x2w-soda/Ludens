#include <Extra/doctest/doctest.h>
#include <Ludens/Asset/AssetRegistry.h>

using namespace LD;

TEST_CASE("AssetRegistry path collisions")
{
    SUIDRegistry idReg = SUIDRegistry::create();
    AssetRegistry reg = AssetRegistry::create();

    AssetEntry entry = reg.register_asset(idReg, ASSET_TYPE_FONT, "font/f1");
    CHECK(entry);

    String str;
    CHECK_FALSE(reg.is_path_valid("font/f1", str));
    CHECK(str == "font/f1");
    entry = reg.register_asset(idReg, ASSET_TYPE_FONT, "font/f1");
    CHECK_FALSE(entry);

    entry = reg.register_asset(idReg, ASSET_TYPE_AUDIO_CLIP, "font/f1");
    CHECK_FALSE(entry);

    AssetRegistry::destroy(reg);
    SUIDRegistry::destroy(idReg);
    CHECK_FALSE(get_memory_leaks(nullptr));
}