#include <Ludens/DSA/Diagnostics.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/System/FileSystem.h>
#include <Ludens/System/FileSystemAsync.h>

#include <format>
#include <fstream>

namespace LD {
namespace FS {

enum TaskStatus : uint32_t
{
    TASK_STATUS_IDLE = 0,
    TASK_STATUS_IN_PROGRESS = 1,
    TASK_STATUS_SUCCESS = 2,
    TASK_STATUS_FAILURE = 3,
};

ReadFileAsync::ReadFileAsync()
    : mStatus(TASK_STATUS_IDLE)
{
}

void ReadFileAsync::begin(const Path& filePath, const MutView& view)
{
    LD_PROFILE_SCOPE;

    DiagnosticScope scope(mDiag, "ReadFileAsync read file to view");

    std::ifstream file(filePath, std::ios::binary);

    if (!file.is_open())
    {
        mDiag.mark_error(std::format("failed to open [{}]", filePath.string()).c_str());
        mStatus.store(TASK_STATUS_FAILURE);
        return;
    }

    file.seekg(0, std::ios::end);
    size_t fileSize = (size_t)file.tellg();

    if (fileSize == 0)
    {
        mStatus.store(TASK_STATUS_SUCCESS);
        return;
    }
    else if (fileSize > view.size)
    {
        mDiag.mark_error(std::format("cant read file of size {} into view of size {}", fileSize, view.size).c_str());
        mStatus.store(TASK_STATUS_FAILURE);
        return;
    }

    file.seekg(0, std::ios::beg);

    mFileSize.store(fileSize);
    mStatus.store(TASK_STATUS_IN_PROGRESS);
    mBytesRead.store(0);

    constexpr size_t chunkSize = 1024 * 1024;
    size_t offset = 0;

    while (offset < view.size)
    {
        size_t toRead = std::min(chunkSize, view.size - offset);

        file.read((char*)view.data + offset, toRead);
        // TODO: check for errors

        offset += toRead;
        mBytesRead.store(offset);
    }

    file.close();

    mBytesRead.store(fileSize);
    mStatus.store(TASK_STATUS_SUCCESS);
}

void ReadFileAsync::begin(const Path& filePath, Vector<byte>& vector)
{
    DiagnosticScope scope(mDiag, "ReadFileAsync read file to vector");
    
    uint64_t fileSize;
    String err;

    if (!FS::get_file_size(filePath, fileSize, err))
    {
        mDiag.mark_error(err);
        mStatus.store(TASK_STATUS_FAILURE);
        return;
    }

    vector.resize(fileSize);
    MutView view((byte*)vector.data(), vector.size());

    begin(filePath, view);
}

float ReadFileAsync::progress() const
{
    size_t fileSize = mFileSize.load();

    if (fileSize == 0)
        return 0.0f;

    return (float)mBytesRead.load() / (float)fileSize;
}

bool ReadFileAsync::has_completed(bool& success, size_t& bytesRead) const
{
    TaskStatus status = (TaskStatus)mStatus.load();

    success = false;
    bytesRead = 0;

    switch (status)
    {
    case TASK_STATUS_SUCCESS:
        success = true;
        [[fallthrough]];
    case TASK_STATUS_IN_PROGRESS:
        bytesRead = mBytesRead.load();
        break;
    default:
        break;
    }

    return status == TASK_STATUS_SUCCESS || status == TASK_STATUS_FAILURE;
}

WriteFileAsync::WriteFileAsync()
    : mStatus(TASK_STATUS_IDLE)
{
}

void WriteFileAsync::begin(const Path& filePath, const View& view)
{
    LD_PROFILE_SCOPE;

    DiagnosticScope scope(mDiag, "WriteFileAsync::begin");

    if (!view.data || view.size == 0)
    {
        mStatus.store(TASK_STATUS_SUCCESS);
        return;
    }

    std::ofstream file(filePath, std::ios::binary);

    if (!file.is_open())
    {
        mDiag.mark_error(std::format("failed to open [{}]", filePath.string()));
        mStatus.store(TASK_STATUS_FAILURE);
        return;
    }

    mStatus.store(TASK_STATUS_IN_PROGRESS);
    mBytesWritten.store(0);
    mFileSize = view.size;

    constexpr size_t chunkSize = 1024 * 1024;
    size_t offset = 0;

    while (offset < view.size)
    {
        size_t toWrite = std::min(chunkSize, view.size - offset);

        file.write((char*)view.data + offset, toWrite);
        // TODO: check for errors

        offset += toWrite;
        mBytesWritten.store(offset);
    }

    file.close();

    mStatus.store(TASK_STATUS_SUCCESS);
}

float WriteFileAsync::progress() const
{
    if (mFileSize == 0)
        return 0.0f;

    return (float)mBytesWritten.load() / (float)mFileSize;
}

bool WriteFileAsync::has_completed(bool& success, size_t& bytesWritten) const
{
    TaskStatus status = (TaskStatus)mStatus.load();

    success = false;
    bytesWritten = 0;

    switch (status)
    {
    case TASK_STATUS_SUCCESS:
        success = true;
        [[fallthrough]];
    case TASK_STATUS_IN_PROGRESS:
        bytesWritten = mBytesWritten.load();
        break;
    default:
        break;
    }

    return status == TASK_STATUS_SUCCESS || status == TASK_STATUS_FAILURE;
}

} // namespace FS
} // namespace LD
