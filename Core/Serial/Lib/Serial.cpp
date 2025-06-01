#include <Ludens/Serial/Serial.h>

namespace LD {

Serializer::Serializer(size_t size, const byte* data)
    : mReadPos(0)
{
    mBuffer.write(data, size);
}

} // namespace LD
