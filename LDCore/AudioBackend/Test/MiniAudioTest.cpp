#include <Extra/doctest/doctest.h>
#include <Ludens/AudioBackend/MiniAudio.h>

using namespace LD;

TEST_CASE("MiniAudio basic")
{
    MiniAudioInfo maI{};
    maI.dataCallback = nullptr;
    maI.userData = nullptr;
    MiniAudio ma = MiniAudio::create(maI);
    CHECK(ma);

    MiniAudio::destroy(ma);
}