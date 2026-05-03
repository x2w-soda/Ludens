#pragma once

/// This is a heavy header, probably only include this
/// only from source files.
#include <Ludens/DSA/Vector.h>
#include <Ludens/DSA/Buffer.h>
#include <Ludens/DSA/String.h>
#include <Ludens/Header/View.h>

#include <string>

namespace LD {

inline View view(const Buffer& buf)
{
    return View(buf.data(), buf.size());
}

inline View view(const String& str)
{
    return View(str.data(), str.size());
}

inline View view(const std::string& str)
{
    return View(reinterpret_cast<const byte*>(str.data()), str.size());
}

inline View view(const Vector<byte>& v)
{
    return View(v.data(), v.size());
}

} // namespace LD