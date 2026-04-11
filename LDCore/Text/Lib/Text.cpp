#include <Ludens/Text/Text.h>

#include <cctype>

namespace LD {

size_t text_find_previous_word(View text, size_t pos)
{
    if (text.size == 0)
        return 0;

    if (pos >= text.size)
        pos = text.size - 1;

    while (pos > 0 && isspace(text.data[pos]))
        pos--;

    while (pos > 0 && !isspace(text.data[pos]))
        pos--;

    if (pos > 0 && isspace(text.data[pos]))
        pos++;

    return pos;
}

} // namespace LD