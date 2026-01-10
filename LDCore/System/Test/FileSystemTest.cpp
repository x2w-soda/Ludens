#include <Extra/doctest/doctest.h>
#include <Ludens/DSA/Diagnostics.h>
#include <Ludens/System/FileSystem.h>
#include <LudensUtil/LudensLFS.h>

#include <string>

using namespace LD;

TEST_CASE("FS exists" * doctest::skip(!LudensLFS::get_directory_path()))
{
    CHECK_FALSE(FS::exists(sLudensLFS.test.nonExistentFilePath));
    CHECK(FS::exists(sLudensLFS.test.emptyFilePath));
    CHECK(FS::exists(FS::current_path()));
}

TEST_CASE("FS is_directory" * doctest::skip(!LudensLFS::get_directory_path()))
{
    CHECK_FALSE(FS::is_directory(sLudensLFS.test.nonExistentFilePath));
    CHECK_FALSE(FS::is_directory(sLudensLFS.test.emptyFilePath));
    CHECK(FS::is_directory(FS::current_path()));
}

TEST_CASE("FS get_file_size" * doctest::skip(!LudensLFS::get_directory_path()))
{
    Diagnostics diag;
    uint64_t fileSize;
    CHECK(FS::get_file_size(sLudensLFS.test.emptyFilePath, fileSize, diag));
    CHECK(fileSize == 0);

    CHECK_FALSE(FS::get_file_size(sLudensLFS.test.nonExistentFilePath, fileSize, diag));
}

TEST_CASE("FS get_positive_file_size" * doctest::skip(!LudensLFS::get_directory_path()))
{
    Diagnostics diag1, diag2;
    uint64_t fileSize;
    CHECK_FALSE(FS::get_positive_file_size(sLudensLFS.test.emptyFilePath, fileSize, diag1));
    CHECK_FALSE(FS::get_positive_file_size(sLudensLFS.test.nonExistentFilePath, fileSize, diag2));
}