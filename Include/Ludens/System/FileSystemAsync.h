#pragma once

#include <Ludens/DSA/Diagnostics.h>
#include <Ludens/DSA/Vector.h>
#include <Ludens/System/FileSystem.h>
#include <atomic>
#include <cstddef>

namespace LD {
namespace FS {

/// @brief Control data for an async file read attempt.
///        Worker thread reads file synchronously in chunks.
///        Main thread observes the task atomically.
class ReadFileAsync
{
public:
    ReadFileAsync();
    ReadFileAsync(const ReadFileAsync&) = delete;
    ReadFileAsync(ReadFileAsync&&) = delete;
    ~ReadFileAsync() = default;

    ReadFileAsync& operator=(const ReadFileAsync&) = delete;
    ReadFileAsync& operator=(ReadFileAsync&&) = delete;

    /// @brief Called on worker thread to begin reading file to view.
    /// @param filePath Source file path.
    /// @param view Worker thread begins writing to mutable view.
    /// @param diag Worker thread writes error upon failure.
    /// @warning Main thread should not access view and diagnostics until task completion.
    void begin(const Path& filePath, const MutView& view);

    /// @brief Called on worker thread to begin reading file to vector.
    /// @warning Main thread should not access vector and diagnostics until task completion.
    void begin(const Path& filePath, Vector<byte>& vector);

    /// @brief Atomically check approximate progress.
    float progress() const;

    /// @brief Atomically check task status.
    bool has_completed(bool& success, size_t& bytesRead) const;

private:
    Diagnostics mDiag{};
    std::atomic_uint32_t mStatus;
    std::atomic_size_t mBytesRead = 0;
    std::atomic_size_t mFileSize = 0;
};

class WriteFileAsync
{
public:
    WriteFileAsync();
    WriteFileAsync(const WriteFileAsync&) = delete;
    WriteFileAsync(WriteFileAsync&&) = delete;
    ~WriteFileAsync() = default;

    WriteFileAsync& operator=(const WriteFileAsync&) = delete;
    WriteFileAsync& operator=(WriteFileAsync&&) = delete;

    /// @brief Called on worker thread to begin reading file to view.
    /// @param filePath Source file path.
    /// @param view Worker thread begins reading from view.
    /// @param diag Worker thread writes error upon failure.
    /// @warning Main thread should not access view and diagnostics until task completion.
    void begin(const Path& filePath, const View& view);

    /// @brief Atomically check approximate progress.
    float progress() const;

    /// @brief Atomically check task status.
    bool has_completed(bool& success, size_t& bytesWritten) const;

private:
    Diagnostics mDiag{};
    size_t mFileSize = 0;
    std::atomic_uint32_t mStatus;
    std::atomic_size_t mBytesWritten = 0;
};

} // namespace FS
} // namespace LD