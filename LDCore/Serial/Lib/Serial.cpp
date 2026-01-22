#include <Ludens/Serial/Serial.h>

namespace LD {

Serializer::Serializer(size_t size)
{
    mBuffer.resize(size);
}

Serializer::~Serializer()
{
    LD_ASSERT(mChunkStack.empty());
}

Deserializer::Deserializer(const void* data, size_t size)
    : mData((const byte*)data), mDataSize(size)
{
}

Deserializer::Deserializer(const View& view)
    : mData((const byte*)view.data), mDataSize(view.size)
{
}

} // namespace LD
