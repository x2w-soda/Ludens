#include <Ludens/Serial/Serial.h>

namespace LD {

Serializer::Serializer(size_t size)
    : mReadPos(0)
{
    mBuffer.resize(size);
}

} // namespace LD
