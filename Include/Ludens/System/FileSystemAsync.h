#pragma once

#include <Ludens/DSA/Vector.h>
#include <Ludens/System/FileSystem.h>
#include <atomic>
#include <cstddef>

namespace LD {
namespace FS {

/// @brief Control data for an async file read attempt.
///        Worker thread reads file synchronously in chunks.
///        Main thread observes the task atomically.
class ReadFileTask
{
public:
    ReadFileTask();
    ReadFileTask(const ReadFileTask&) = delete;
    ReadFileTask(ReadFileTask&&) = delete;
    ~ReadFileTask() = default;

    ReadFileTask& operator=(const ReadFileTask&) = delete;
    ReadFileTask& operator=(ReadFileTask&&) = delete;

    /// @brief Called on worker thread to begin reading file to view.
    /// @param filePath Source file path.
    /// @param view Worker thread begins writing to mutable view.
    /// @param diag Worker thread writes error upon failure.
    /// @warning Main thread should not access view and diagnostics until task completion.
    void begin(const Path& filePath, const MutView& view, Diagnostics& diag);

    /// @brief Called on worker thread to begin reading file to vector.
    /// @warning Main thread should not access vector and diagnostics until task completion.
    void begin(const Path& filePath, Vector<byte>& vector, Diagnostics& diag);

    /// @brief Atomically check approximate progress.
    float progress() const;

    /// @brief Atomically check task status.
    bool has_completed(bool& success, size_t& bytesRead) const;

private:
    Diagnostics* mDiag = nullptr;
    std::atomic_uint32_t mStatus;
    std::atomic_size_t mBytesRead = 0;
    std::atomic_size_t mFileSize = 0;
};

class WriteFileTask
{
public:
    WriteFileTask();
    WriteFileTask(const WriteFileTask&) = delete;
    WriteFileTask(WriteFileTask&&) = delete;
    ~WriteFileTask() = default;

    WriteFileTask& operator=(const WriteFileTask&) = delete;
    WriteFileTask& operator=(WriteFileTask&&) = delete;

    /// @brief Called on worker thread to begin reading file to view.
    /// @param filePath Source file path.
    /// @param view Worker thread begins reading from view.
    /// @param diag Worker thread writes error upon failure.
    /// @warning Main thread should not access view and diagnostics until task completion.
    void begin(const Path& filePath, const View& view, Diagnostics& diag);

    /// @brief Atomically check approximate progress.
    float progress() const;

    /// @brief Atomically check task status.
    bool has_completed(bool& success, size_t& bytesWritten) const;

private:
    Diagnostics* mDiag = nullptr;
    size_t mFileSize = 0;
    std::atomic_uint32_t mStatus;
    std::atomic_size_t mBytesWritten = 0;
};

} // namespace FS
} // namespace LD