#include <Extra/doctest/doctest.h>
#include <Ludens/Asset/AssetRegistry.h>

using namespace LD;

TEST_CASE("AssetRegistry URI collisions")
{
    SUIDRegistry idReg = SUIDRegistry::create();
    AssetRegistry reg = AssetRegistry::create();

    AssetEntry entry = reg.register_asset(idReg, ASSET_TYPE_FONT, "ld://font/f1");
    CHECK(entry);

    entry = reg.register_asset(idReg, ASSET_TYPE_FONT, "ld://font/f1");
    CHECK_FALSE(entry);

    entry = reg.register_asset(idReg, ASSET_TYPE_AUDIO_CLIP, "ld://font/f1");
    CHECK_FALSE(entry);

    AssetRegistry::destroy(reg);
    SUIDRegistry::destroy(idReg);
    CHECK_FALSE(get_memory_leaks(nullptr));
}