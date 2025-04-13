#pragma once

#include <Ludens/Header/Class.h>
#include <Ludens/DSA/Buffer.h>
#include <Ludens/System/Memory.h>
#include <cstdint>

namespace LD {

class Serializer : public NonCopyable
{
public:
    Serializer();

	inline void write_u16(uint16_t u16)
	{
        char bytes[2];
        bytes[0] = u16 & 0xFF;
        bytes[1] = (u16 >> 8) & 0xFF;
		mBuffer.write(bytes, 2);
	}

    inline void write_u32(uint32_t u32)
    {
        char bytes[4];
        bytes[0] = u32 & 0xFF;
        bytes[1] = (u32 >> 8) & 0xFF;
        bytes[2] = (u32 >> 16) & 0xFF;
        bytes[3] = (u32 >> 24) & 0xFF;
        mBuffer.write(bytes, 4);
    }

    inline void write_f32(float f) { write_u32(*(uint32_t*)&f); }

	inline void read_u16(uint16_t& u16)
	{
		char* bytes = (char*)mBuffer.read(mReadPos);
		u16 = 0;
        u16 |= bytes[0];
        u16 |= bytes[1] << 8;
		mReadPos += 2;
	}

    inline void read_u32(uint32_t& u32)
    {
        char* bytes = (char*)mBuffer.read(mReadPos);
        u32 = 0;
        u32 |= bytes[0];
        u32 |= bytes[1] << 8;
        u32 |= bytes[2] << 16;
        u32 |= bytes[3] << 24;
        mReadPos += 4;
    }

    inline void read_f32(float& f) { read_u32(*(uint32_t*)&f); }

    inline size_t size() const { return mBuffer.size(); }

private:
    size_t mReadPos = 0;
    Buffer mBuffer;
};

template <typename T>
inline void serialize(Serializer& serializer, const T& serialObject)
{
    T::serialize(serializer, serialObject);
}

template <typename T>
inline void deserialize(Serializer& serializer, T& serialObject)
{
    T::deserialize(serializer, serialObject);
}

} // namespace LD
