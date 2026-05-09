#include "LineBuffer.h"

namespace LD {

void LineBuffer::append(const String& line)
{
    std::lock_guard<std::mutex> lock(mMutex);

    mLines.push_back(line);
}

Vector<String> LineBuffer::extract()
{
    Vector<String> lines;

    {
        std::lock_guard<std::mutex> lock(mMutex);
        lines.swap(mLines);
    }

    return lines;
}

} // namespace LD