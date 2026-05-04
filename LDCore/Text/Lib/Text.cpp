#include <Ludens/Text/Text.h>

#include <cctype>
#include <string_view>

namespace LD {

Vector<Range> text_split_ranges(View text, char delimiter, bool includeDelimiter)
{
    Vector<Range> ranges;

    if (text.size == 0)
        return ranges;

    std::string_view view((const char*)text.data, text.size);
    size_t start = 0;
    size_t end = view.find(delimiter);

    while (end != std::string_view::npos)
    {
        if (end > start)
            ranges.emplace_back(start, end - start);
        if (includeDelimiter)
            ranges.emplace_back(end, 1);
        start = end + 1;
        end = view.find(delimiter, start);
    }

    if (text.size > start)
        ranges.emplace_back(start, text.size - start);

    return ranges;
}

Vector<View> text_split_views(View text, char delimiter, bool includeDelimiter)
{
    Vector<Range> ranges = text_split_ranges(text, delimiter, includeDelimiter);
    Vector<View> views(ranges.size());

    for (size_t i = 0; i < views.size(); i++)
    {
        views[i].data = text.data + ranges[i].offset;
        views[i].size = ranges[i].size;
    }

    return views;
}

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