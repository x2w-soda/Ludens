#include <Ludens/AudioBackend/MiniAudio.h>
#include <Extra/doctest/doctest.h>

using namespace LD;

TEST_CASE("MiniAudio basic")
{
    MiniAudio ma = MiniAudio::create();
    CHECK(ma);

    MiniAudio::destroy(ma);
}