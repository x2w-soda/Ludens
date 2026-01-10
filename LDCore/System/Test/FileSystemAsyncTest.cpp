#include <Extra/doctest/doctest.h>
#include <Ludens/DSA/Diagnostics.h>
#include <Ludens/System/FileSystemAsync.h>
#include <LudensUtil/LudensLFS.h>

#include <format>
#include <iostream>
#include <string>
#include <thread>

using namespace LD;

static void worker_read_file_to_vector(FS::ReadFileTask* task, const FS::Path& path, Vector<byte>* vec, Diagnostics* diag)
{
    task->begin(path, *vec, *diag);
}

static void worker_write_file(FS::WriteFileTask* task, const FS::Path& path, const View& view, Diagnostics* diag)
{
    task->begin(path, view, *diag);
}

TEST_CASE("ReadFileTask bad path" * doctest::skip(!LudensLFS::get_directory_path()))
{
    Diagnostics diag;
    FS::ReadFileTask task;
    FS::Path path = "definitelyDoesNotExist.txt";
    REQUIRE(!FS::exists(path));

    Vector<byte> vec;
    std::thread worker(&worker_read_file_to_vector, &task, path, &vec, &diag);
    std::string err;

    bool success;
    size_t bytesRead;
    while (!task.has_completed(success, bytesRead))
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    CHECK(!success);
    CHECK(bytesRead == 0);
    CHECK(vec.empty());
    CHECK(diag.get_error(err));

    worker.join();
}

TEST_CASE("ReadFileTask empty file" * doctest::skip(!LudensLFS::get_directory_path()))
{
    Diagnostics diag;
    FS::ReadFileTask task;
    FS::Path path = sLudensLFS.test.emptyFilePath;
    std::string err;
    uint64_t fileSize;
    REQUIRE(FS::get_file_size(path, fileSize, err));
    REQUIRE(fileSize == 0);

    Vector<byte> vec;
    std::thread worker(&worker_read_file_to_vector, &task, path, &vec, &diag);

    bool success;
    size_t bytesRead;
    while (!task.has_completed(success, bytesRead))
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    CHECK(success);
    CHECK(bytesRead == 0);
    CHECK(vec.size() == 0);
    CHECK(!diag.get_error(err));

    worker.join();
}

TEST_CASE("ReadFileTask vector" * doctest::skip(!LudensLFS::get_directory_path()))
{
    Diagnostics diag;
    FS::ReadFileTask task;
    Vector<byte> vec;

    uint64_t fileSize;
    FS::Path path = sLudensLFS.materialIconsPath;
    REQUIRE(FS::get_positive_file_size(path, fileSize, diag));

    std::thread worker(&worker_read_file_to_vector, &task, path, &vec, &diag);

    bool success;
    size_t bytesRead;
    while (!task.has_completed(success, bytesRead))
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    CHECK(success);
    CHECK(vec.size() == fileSize);
    CHECK(bytesRead == fileSize);

    worker.join();
}

TEST_CASE("WriteFileTask empty view" * doctest::skip(!LudensLFS::get_directory_path()))
{
    Diagnostics diag;
    FS::WriteFileTask task;
    FS::Path path = "definitelyDoesNotExist.txt";
    REQUIRE(!FS::exists(path));

    View emptyView{};
    std::thread worker(&worker_write_file, &task, path, emptyView, &diag);

    bool success;
    size_t bytesWritten;
    while (!task.has_completed(success, bytesWritten))
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    CHECK(success);
    CHECK(bytesWritten == 0);

    worker.join();
}

TEST_CASE("WriteFileTask" * doctest::skip(!LudensLFS::get_directory_path()))
{
    Diagnostics diag;
    FS::WriteFileTask task;
    FS::Path path = "foo.txt";
    REQUIRE(!FS::exists(path));

    std::string str = "foo";
    View view(str.data(), str.size());
    
    std::thread worker(&worker_write_file, &task, path, view, &diag);

    bool success;
    size_t bytesWritten;
    while (!task.has_completed(success, bytesWritten))
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    CHECK(success);
    CHECK(bytesWritten == str.size());

    worker.join();

    FS::remove(path, str);
}