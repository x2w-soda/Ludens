#include <Ludens/Header/Range.h>
#include <type_traits>

using namespace LD;

static_assert(std::is_trivial_v<Range>);