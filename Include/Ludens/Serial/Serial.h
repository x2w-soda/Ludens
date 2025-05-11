#pragma once

#include <Ludens/DSA/Buffer.h>
#include <Ludens/Header/Class.h>
#include <Ludens/Header/Math/Vec4.h>
#include <Ludens/System/Memory.h>
#include <cstdint>

namespace LD {

class Serializer : public NonCopyable
{
public:
    Serializer();
    Serializer(size_t size, const byte* data);

    inline void write(const byte* bytes, size_t size)
    {
        mBuffer.write(bytes, size);
    }

    inline void write_u16(uint16_t u16)
    {
        byte bytes[2];
        bytes[0] = u16 & 0xFF;
        bytes[1] = (u16 >> 8) & 0xFF;
        mBuffer.write(bytes, 2);
    }

    inline void write_u32(uint32_t u32)
    {
        byte bytes[4];
        bytes[0] = u32 & 0xFF;
        bytes[1] = (u32 >> 8) & 0xFF;
        bytes[2] = (u32 >> 16) & 0xFF;
        bytes[3] = (u32 >> 24) & 0xFF;
        mBuffer.write(bytes, 4);
    }

    inline void write_i32(int32_t i32)
    {
        byte bytes[4];
        bytes[0] = i32 & 0xFF;
        bytes[1] = (i32 >> 8) & 0xFF;
        bytes[2] = (i32 >> 16) & 0xFF;
        bytes[3] = (i32 >> 24) & 0xFF;
        mBuffer.write(bytes, 4);
    }

    inline void write_f32(float f)
    {
        mBuffer.write((byte*)&f, 4);
    }

    inline void write_vec2(const Vec2& v)
    {
        mBuffer.write((byte*)&v.x, 4);
        mBuffer.write((byte*)&v.y, 4);
    }

    inline void write_vec3(const Vec3& v)
    {
        mBuffer.write((byte*)&v.x, 4);
        mBuffer.write((byte*)&v.y, 4);
        mBuffer.write((byte*)&v.z, 4);
    }

    inline void write_vec4(const Vec4& v)
    {
        mBuffer.write((byte*)&v.x, 4);
        mBuffer.write((byte*)&v.y, 4);
        mBuffer.write((byte*)&v.z, 4);
        mBuffer.write((byte*)&v.w, 4);
    }

    inline void read(byte* bytes, size_t size)
    {
        const byte* from = (const byte*)mBuffer.read(mReadPos);
        memcpy(bytes, from, size);
        mReadPos += size;
    }

    inline void read_u16(uint16_t& u16)
    {
        byte* bytes = (byte*)mBuffer.read(mReadPos);
        u16 = 0;
        u16 |= bytes[0];
        u16 |= bytes[1] << 8;
        mReadPos += 2;
    }

    inline void read_u32(uint32_t& u32)
    {
        byte* bytes = (byte*)mBuffer.read(mReadPos);
        u32 = 0;
        u32 |= bytes[0];
        u32 |= bytes[1] << 8;
        u32 |= bytes[2] << 16;
        u32 |= bytes[3] << 24;
        mReadPos += 4;
    }

    inline void read_i32(int32_t& i32)
    {
        byte* bytes = (byte*)mBuffer.read(mReadPos);
        i32 = 0;
        i32 |= bytes[0];
        i32 |= bytes[1] << 8;
        i32 |= bytes[2] << 16;
        i32 |= bytes[3] << 24;
        mReadPos += 4;
    }

    inline void read_f32(float& f)
    {
        f = *(float*)mBuffer.read(mReadPos);
        mReadPos += 4;
    }

    inline void read_vec2(Vec2& v)
    {
        v.x = *(float*)mBuffer.read(mReadPos);
        v.y = *(float*)mBuffer.read(mReadPos + 4);
        mReadPos += 8;
    }

    inline void read_vec3(Vec3& v)
    {
        v.x = *(float*)mBuffer.read(mReadPos);
        v.y = *(float*)mBuffer.read(mReadPos + 4);
        v.z = *(float*)mBuffer.read(mReadPos + 8);
        mReadPos += 12;
    }

    inline void read_vec4(Vec4& v)
    {
        v.x = *(float*)mBuffer.read(mReadPos);
        v.y = *(float*)mBuffer.read(mReadPos + 4);
        v.z = *(float*)mBuffer.read(mReadPos + 8);
        v.w = *(float*)mBuffer.read(mReadPos + 12);
        mReadPos += 16;
    }

    inline size_t size() const { return mBuffer.size(); }

    inline const byte* view(size_t& size) const
    {
        size = mBuffer.size();
        return mBuffer.data();
    }

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
