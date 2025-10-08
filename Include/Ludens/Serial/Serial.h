#pragma once

#include <Ludens/DSA/Buffer.h>
#include <Ludens/Header/Class.h>
#include <Ludens/Header/Math/Vec4.h>
#include <Ludens/Serial/Endianness.h>
#include <Ludens/System/Memory.h>
#include <cstdint>
#include <cstring>

namespace LD {

class Serializer : public NonCopyable
{
public:
    Serializer() = default;

    /// @brief Create buffer with fixed size
    /// @param size Initial buffer byte size
    Serializer(size_t size);

    inline void write(const byte* bytes, size_t size)
    {
        mBuffer.write(bytes, size);
    }

    inline void write_u8(uint8_t u8)
    {
        mBuffer.write(&u8, 1);
    }

    inline void write_u16(uint16_t u16)
    {
        byte bytes[2];
        le_u16_to_bytes(u16, bytes);
        mBuffer.write(bytes, 2);
    }

    inline void write_u32(uint32_t u32)
    {
        byte bytes[4];
        le_u32_to_bytes(u32, bytes);
        mBuffer.write(bytes, 4);
    }

    inline void write_u64(uint64_t u64)
    {
        byte bytes[8];
        le_u64_to_bytes(u64, bytes);
        mBuffer.write(bytes, 8);
    }

    inline void write_i8(int8_t i8)
    {
        mBuffer.write((byte*)&i8, 1);
    }

    inline void write_i16(int16_t i16)
    {
        byte bytes[2];
        le_i16_to_bytes(i16, bytes);
        mBuffer.write(bytes, 2);
    }

    inline void write_i32(int32_t i32)
    {
        byte bytes[4];
        le_i32_to_bytes(i32, bytes);
        mBuffer.write(bytes, 4);
    }

    inline void write_i64(int64_t i64)
    {
        byte bytes[8];
        le_i64_to_bytes(i64, bytes);
        mBuffer.write(bytes, 8);
    }

    inline void write_f32(float f)
    {
        mBuffer.write((byte*)&f, 4);
    }

    inline void write_f64(double d)
    {
        mBuffer.write((byte*)&d, 8);
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

    inline void read_u8(uint8_t& u8)
    {
        u8 = *(uint8_t*)mBuffer.read(mReadPos++);
    }

    inline void read_u16(uint16_t& u16)
    {
        byte* bytes = (byte*)mBuffer.read(mReadPos);
        le_bytes_to_u16(bytes, u16);
        mReadPos += 2;
    }

    inline void read_u32(uint32_t& u32)
    {
        byte* bytes = (byte*)mBuffer.read(mReadPos);
        le_bytes_to_u32(bytes, u32);
        mReadPos += 4;
    }

    inline void read_u64(uint64_t& u64)
    {
        byte* bytes = (byte*)mBuffer.read(mReadPos);
        le_bytes_to_u64(bytes, u64);
        mReadPos += 8;
    }

    inline void read_i8(int8_t& i8)
    {
        i8 = *(int8_t*)mBuffer.read(mReadPos++);
    }

    inline void read_i16(int16_t& i16)
    {
        byte* bytes = (byte*)mBuffer.read(mReadPos);
        le_bytes_to_i16(bytes, i16);
        mReadPos += 2;
    }

    inline void read_i32(int32_t& i32)
    {
        byte* bytes = (byte*)mBuffer.read(mReadPos);
        le_bytes_to_i32(bytes, i32);
        mReadPos += 4;
    }

    inline void read_i64(int64_t& i64)
    {
        byte* bytes = (byte*)mBuffer.read(mReadPos);
        le_bytes_to_i64(bytes, i64);
        mReadPos += 8;
    }

    inline void read_f32(float& f)
    {
        f = *(float*)mBuffer.read(mReadPos);
        mReadPos += 4;
    }

    inline void read_f64(double& d)
    {
        d = *(double*)mBuffer.read(mReadPos);
        mReadPos += 8;
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

    /// @brief Get underlying buffer size in bytes.
    inline size_t size() const { return mBuffer.size(); }

    /// @brief Get writable pointer of underlying buffer.
    inline byte* data() { return mBuffer.data(); }

    /// @brief Get read only view of the underlying buffer.
    inline const byte* view(size_t& size) const
    {
        size = mBuffer.size();
        return mBuffer.data();
    }

    /// @brief Get the current buffer position for reading.
    inline const byte* view_now() const
    {
        return mBuffer.data() + mReadPos;
    }

    /// @brief advance the read pointer
    /// @param dist number of bytes to advance
    inline void advance(size_t dist)
    {
        mReadPos += dist;
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
