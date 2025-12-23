#include <Ludens/Header/Platform.h>
#ifdef LD_PLATFORM_WIN32

#include <Ludens/Header/Assert.h>
#include <Ludens/Log/Log.h>
#include <Ludens/Media/Format/ICO.h>
#include <Ludens/Media/Win32Struct.h>
#include <LudensBuilder/Win32Util/Win32Util.h>
#include <format>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <WinBase.h>

namespace LD {

static Log sLog("LDBuilder");

Win32Util Win32Util::create()
{
    return {};
}

void Win32Util::destroy(Win32Util util)
{
    (void*)util;
}

bool Win32Util::patch_icon_resources(const FS::Path& path, const FS::Path& icoPath)
{
    std::string icoPathStr = path.string();

    std::vector<byte> icoData;
    if (!FS::read_file_to_vector(icoPath, icoData))
    {
        sLog.error("failed to open {}", icoPathStr);
        return false;
    }

    const ICONDIR* iconDir = (const ICONDIR*)icoData.data();
    if (iconDir->idReserved != 0 || iconDir->idType != 1)
    {
        sLog.error("invalid icon file {}", icoPathStr);
        return false;
    }

    const ICONDIRENTRY* entries = (ICONDIRENTRY*)(icoData.data() + sizeof(ICONDIR));

    HANDLE handle = BeginUpdateResourceA(path.string().c_str(), FALSE);
    if (!handle)
    {
        sLog.error("BeginUpdateResourceA failed");
        return false;
    }

    const WORD lang = MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL);
    constexpr WORD iconBaseID = 1;
    constexpr WORD iconGroupID = 1;

    // update individual RT_ICON

    sLog.info("updating RT_ICON entries, found {} ICONDIRENTRY in [{}]", iconDir->idCount, icoPath.string());

    for (uint16_t i = 0; i < iconDir->idCount; i++)
    {
        const ICONDIRENTRY& e = entries[i];

        if (e.dwImageOffset + e.dwBytesInRes > icoData.size())
        {
            EndUpdateResourceA(handle, TRUE);
            throw std::runtime_error("Corrupt ICONDIRENTRY");
            sLog.error("bad ICONDIRENTRY, bitmap range [{}, {}] invalid for file size {}", e.dwImageOffset, e.dwBytesInRes, icoData.size());
            return false;
        }

        const void* bitmapData = icoData.data() + e.dwImageOffset;
        uint64_t bitmapSize = e.dwBytesInRes;
        WORD iconID = iconBaseID + i;

        if (!UpdateResourceA(handle, RT_ICON, MAKEINTRESOURCEA(iconID), lang, (LPVOID)bitmapData, (DWORD)bitmapSize))
        {
            EndUpdateResourceA(handle, TRUE);
            sLog.error("EndUpdateResourceA failed for RT_ICON");
            return false;
        }

        sLog.info("updated ({}x{}) RT_ICON at ID {}", e.bWidth, e.bHeight, iconID);
    }

    // update RT_GROUP_ICON

    size_t grpSize = sizeof(GRPICONDIR) + iconDir->idCount * sizeof(GRPICONDIRENTRY);
    std::vector<byte> grpData(grpSize);

    GRPICONDIR* grpDir = (GRPICONDIR*)grpData.data();
    grpDir->idReserved = 0;
    grpDir->idType = 1;
    grpDir->idCount = iconDir->idCount;

    GRPICONDIRENTRY* grpEntries = (GRPICONDIRENTRY*)(grpData.data() + sizeof(GRPICONDIR));
    for (uint16_t i = 0; i < iconDir->idCount; ++i)
    {
        const ICONDIRENTRY& src = entries[i];
        GRPICONDIRENTRY& dst = grpEntries[i];

        dst.bWidth = src.bWidth;
        dst.bHeight = src.bHeight;
        dst.bColorCount = src.bColorCount;
        dst.bReserved = src.bReserved;
        dst.wPlanes = src.wPlanes;
        dst.wBitCount = src.wBitCount;
        dst.dwBytesInRes = src.dwBytesInRes;
        dst.nId = iconBaseID + i; // corresponding RT_ICON id
    }

    if (!UpdateResourceA(handle, RT_GROUP_ICON, MAKEINTRESOURCEA(iconGroupID), lang, grpData.data(), static_cast<DWORD>(grpData.size())))
    {
        EndUpdateResourceA(handle, TRUE);
        sLog.error("UpdateResourceA failed for RT_GROUP_ICON");
        return false;
    }

    // commit changes

    if (!EndUpdateResourceA(handle, FALSE))
    {
        sLog.error("EndUpdateResourceW failed");
        return false;
    }

    return true;
}

} // namespace LD
#endif // LD_PLATFORM_WIN32