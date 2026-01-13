#include <Ludens/Header/Platform.h>
#include <Ludens/Log/Log.h>
#ifdef LD_PLATFORM_WIN32

#include "./Window.h"

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <dwmapi.h>

#pragma comment(lib, "dwmapi.lib")

namespace LD {

static Log sLog("Application");

static COLORREF to_colorref(Color color)
{
    COLORREF colorRef = 0;
    colorRef |= (color >> 24) & 0xFF;
    colorRef |= ((color >> 16) & 0xFF) << 8;
    colorRef |= ((color >> 8) & 0xFF) << 16;

    return colorRef;
}

void WindowObj::hint_border_color(Color color)
{
    HWND hwnd = glfwGetWin32Window(mHandle);
    COLORREF colorRef = to_colorref(color);
    HRESULT result = DwmSetWindowAttribute(hwnd, DWMWA_BORDER_COLOR, &colorRef, sizeof(colorRef));

    if (result != S_OK)
    {
        sLog.warn("Win32 DwmSetWindowAttribute DWMWA_BORDER_COLOR failed with {}", (long)result);
    }
}

void WindowObj::hint_title_bar_color(Color color)
{
    HWND hwnd = glfwGetWin32Window(mHandle);
    COLORREF colorRef = to_colorref(color);
    HRESULT result = DwmSetWindowAttribute(hwnd, DWMWA_CAPTION_COLOR, &colorRef, sizeof(colorRef));

    if (result != S_OK)
    {
        sLog.warn("Win32 DwmSetWindowAttribute DWMWA_CAPTION_COLOR failed with {}", (long)result);
    }
}

void WindowObj::hint_title_bar_text_color(Color color)
{
    HWND hwnd = glfwGetWin32Window(mHandle);
    COLORREF colorRef = to_colorref(color);
    HRESULT result = DwmSetWindowAttribute(hwnd, DWMWA_TEXT_COLOR, &colorRef, sizeof(colorRef));

    if (result != S_OK)
    {
        sLog.warn("Win32 DwmSetWindowAttribute DWMWA_TEXT_COLOR failed with {}", (long)result);
    }
}

} // namespace LD

#endif // LD_PLATFORM_WIN32