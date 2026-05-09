#include <Extra/doctest/doctest.h>
#include <Ludens/DSA/Diagnostics.h>
#include <Ludens/System/FileSystemAsync.h>
#include <LudensUtil/LudensLFS/LudensLFS.h>

#include <format>
#include <iostream>
#include <string>
#include <thread>

using namespace LD;

static void worker_read_file_to_vector(FS::ReadFileAsync* task, const FS::Path& path, Vector<byte>* vec)
{
    task->begin(path, *vec);
}

static void worker_write_file(FS::WriteFileAsync* task, const FS::Path& path, const View& view)
{
    task->begin(path, view);
}

TEST_CASE("ReadFileAsync bad path" * doctest::skip(!LudensLFS::get_directory_path()))
{
    FS::ReadFileAsync task;
    FS::Path path = "definitelyDoesNotExist.txt";
    REQUIRE(!FS::exists(path));

    Vector<byte> vec;
    std::thread worker(&worker_read_file_to_vector, &task, path, &vec);
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

    worker.join();
}

TEST_CASE("ReadFileAsync empty file" * doctest::skip(!LudensLFS::get_directory_path()))
{
    FS::ReadFileAsync task;
    FS::Path path = sLudensLFS.test.emptyFilePath;
    String err;
    uint64_t fileSize;
    REQUIRE(FS::get_file_size(path, fileSize, err));
    REQUIRE(fileSize == 0);

    Vector<byte> vec;
    std::thread worker(&worker_read_file_to_vector, &task, path, &vec);

    bool success;
    size_t bytesRead;
    while (!task.has_completed(success, bytesRead))
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    CHECK(success);
    CHECK(bytesRead == 0);
    CHECK(vec.size() == 0);

    worker.join();
}

TEST_CASE("ReadFileAsync vector" * doctest::skip(!LudensLFS::get_directory_path()))
{
    Diagnostics diag;
    FS::ReadFileAsync task;
    Vector<byte> vec;

    uint64_t fileSize;
    FS::Path path = sLudensLFS.editorIconAtlasPath;
    REQUIRE(FS::get_positive_file_size(path, fileSize, diag));

    std::thread worker(&worker_read_file_to_vector, &task, path, &vec);

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

TEST_CASE("WriteFileAsync empty view" * doctest::skip(!LudensLFS::get_directory_path()))
{
    Diagnostics diag;
    FS::WriteFileAsync task;
    FS::Path path = "definitelyDoesNotExist.txt";
    REQUIRE(!FS::exists(path));

    View emptyView{};
    std::thread worker(&worker_write_file, &task, path, emptyView);

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

TEST_CASE("WriteFileAsync" * doctest::skip(!LudensLFS::get_directory_path()))
{
    FS::WriteFileAsync task;
    FS::Path path = "foo.txt";
    REQUIRE(!FS::exists(path));

    String str = "foo";
    View view(str.data(), str.size());
    
    std::thread worker(&worker_write_file, &task, path, view);

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