#include <Ludens/Log/Log.h>
#include <Ludens/Media/Format/GLTF.h>
#include <Ludens/Media/Format/PNG.h>

#include <cctype>

#include "FileTest.h"

namespace LD {

static Log sLog("LDBuilder");

static bool str_equal(const char* lhs, const char* rhs);
static bool check_by_extension(const std::vector<byte>& fileData, const FS::Path& fileExt);
static bool check_png(const std::vector<byte>& fileData);
static bool check_gltf(const std::vector<byte>& fileData);

/// @brief Case insenstive compare between two C strings.
static bool str_equal(const char* lhs, const char* rhs)
{
    if (!lhs || !rhs)
        return false;

    int i = 0;
    while (lhs[i] && rhs[i] && (tolower(lhs[i]) == tolower(rhs[i])))
        i++;

    return !lhs[i] && !rhs[i];
}

static bool check_by_extension(const std::vector<byte>& fileData, const FS::Path& fileExt)
{
    std::string extStr = fileExt.string();
    const char* extCstr = extStr.c_str();

    if (str_equal(extCstr, ".png") && check_png(fileData))
        return true;

    if (str_equal(extCstr, ".gltf") && check_gltf(fileData))
        return true;

    sLog.info("unhandled extension [{}]", extStr);
    return false;
}

static bool check_png(const std::vector<byte>& fileBytes)
{
    if (!PNGData::test_magic(fileBytes.data(), fileBytes.size()))
    {
        sLog.info("invalid PNG data");
        return false;
    }

    sLog.info("PNG image");
    return true;
}

static bool check_gltf(const std::vector<byte>& fileBytes)
{
    std::string summary, error;

    if (!print_gltf_data({(const char*)fileBytes.data(), fileBytes.size()}, summary, error))
    {
        sLog.info("failed to parse GLTF:\n{}", error);
        return false;
    }

    sLog.info("GLTF data:\n{}", summary);
    return true;
}

//
// Public API
//

void FileTest::check_file(const FS::Path& filePath)
{
    if (!FS::exists(filePath))
    {
        sLog.info("file [{}] does not exist", filePath.string());
        return;
    }

    if (FS::is_directory(filePath))
    {
        sLog.info("[{}] is a directory", filePath.string());
        return;
    }

    std::string err;
    std::vector<byte> fileData;
    if (!FS::read_file_to_vector(filePath, fileData, err))
    {
        sLog.info("{}", err);
        return;
    }

    if (filePath.has_extension() && check_by_extension(fileData, filePath.extension()))
        return;

    // TODO: missing file extension, iterate over all recognized formats?
}

} // namespace LD