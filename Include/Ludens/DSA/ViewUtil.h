#pragma once

/// THIS IS A HEAVY HEADER, include this only from source files.
#include <Ludens/DSA/Buffer.h>
#include <Ludens/DSA/String.h>
#include <Ludens/DSA/Vector.h>
#include <Ludens/Header/KeyCode.h>
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

View to_string(KeyCode key);
bool from_string(View str, KeyCode& key);

View to_string(MouseButton btn);
bool from_string(View str, MouseButton& btn);

} // namespace LD