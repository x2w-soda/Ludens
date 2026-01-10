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

ReadFileTask::ReadFileTask()
    : mStatus(TASK_STATUS_IDLE)
{
}

void ReadFileTask::begin(const Path& filePath, const MutView& view, Diagnostics& diag)
{
    LD_PROFILE_SCOPE;

    DiagnosticScope scope(diag, "ReadFileTask read file to view");

    mDiag = &diag;

    std::ifstream file(filePath, std::ios::binary);

    if (!file.is_open())
    {
        diag.mark_error(std::format("failed to open [{}]", filePath.string()));
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
        diag.mark_error(std::format("cant read file of size {} into view of size {}", fileSize, view.size));
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

        file.read(view.data + offset, toRead);
        // TODO: check for errors

        offset += toRead;
        mBytesRead.store(offset);
    }

    file.close();

    mBytesRead.store(fileSize);
    mStatus.store(TASK_STATUS_SUCCESS);
}

void ReadFileTask::begin(const Path& filePath, Vector<byte>& vector, Diagnostics& diag)
{
    DiagnosticScope scope(diag, "ReadFileTask read file to vector");
    
    uint64_t fileSize;
    std::string err;

    if (!FS::get_file_size(filePath, fileSize, err))
    {
        diag.mark_error(err);
        mStatus.store(TASK_STATUS_FAILURE);
        return;
    }

    vector.resize(fileSize);
    MutView view((char*)vector.data(), vector.size());

    begin(filePath, view, diag);
}

float ReadFileTask::progress() const
{
    size_t fileSize = mFileSize.load();

    if (fileSize == 0)
        return 0.0f;

    return (float)mBytesRead.load() / (float)fileSize;
}

bool ReadFileTask::has_completed(bool& success, size_t& bytesRead) const
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
    }

    return status == TASK_STATUS_SUCCESS || status == TASK_STATUS_FAILURE;
}

WriteFileTask::WriteFileTask()
    : mStatus(TASK_STATUS_IDLE)
{
}

void WriteFileTask::begin(const Path& filePath, const View& view, Diagnostics& diag)
{
    LD_PROFILE_SCOPE;

    DiagnosticScope scope(diag, "WriteFileTask::begin");

    mDiag = &diag;

    if (!view.data || view.size == 0)
    {
        mStatus.store(TASK_STATUS_SUCCESS);
        return;
    }

    std::ofstream file(filePath, std::ios::binary);

    if (!file.is_open())
    {
        diag.mark_error(std::format("failed to open [{}]", filePath.string()));
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

        file.write(view.data + offset, toWrite);
        // TODO: check for errors

        offset += toWrite;
        mBytesWritten.store(offset);
    }

    file.close();

    mStatus.store(TASK_STATUS_SUCCESS);
}

float WriteFileTask::progress() const
{
    if (mFileSize == 0)
        return 0.0f;

    return (float)mBytesWritten.load() / (float)mFileSize;
}

bool WriteFileTask::has_completed(bool& success, size_t& bytesWritten) const
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
    }

    return status == TASK_STATUS_SUCCESS || status == TASK_STATUS_FAILURE;
}

} // namespace FS
} // namespace LD