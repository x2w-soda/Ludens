#include <Ludens/DSA/Vector.h>
#include <Ludens/Header/Range.h>
#include <Ludens/Header/View.h>

namespace LD {

Vector<Range> text_split_ranges(View text, char delimiter, bool includeDelimiter = false);
size_t text_find_previous_word(View text, size_t pos);

} // namespace LD