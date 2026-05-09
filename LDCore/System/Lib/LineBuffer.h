#pragma once

#include <Ludens/DSA/String.h>
#include <Ludens/DSA/Vector.h>

#include <mutex>

namespace LD {

/// @brief Thread safe line accumulator.
class LineBuffer
{
public:
    void append(const String& line);
    Vector<String> extract();

private:
    std::mutex mMutex;
    Vector<String> mLines;
};

} // namespace LD