#pragma once


#include <Sts1CobcSw/FileSystem/LfsMemoryDevice.hpp>
#include <Sts1CobcSw/Outcome/Outcome.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>

#include <littlefs/lfs.h>

#include <etl/string.h>

#include <array>
#include <cstddef>
#include <iterator>
#include <span>


namespace sts1cobcsw::fs
{
using Path = etl::string<maxPathLength>;


class File;
class DirectoryIterator;


[[nodiscard]] auto Mount() -> Result<void>;
[[nodiscard]] auto Unmount() -> Result<void>;
[[nodiscard]] auto Open(Path const & path, unsigned int flags) -> Result<File>;
[[nodiscard]] auto CreateDirectory(Path const & path) -> Result<void>;
[[nodiscard]] auto Remove(Path const & path) -> Result<void>;
[[nodiscard]] auto ForceRemove(Path const & path) -> Result<void>;
[[nodiscard]] auto MakeIterator(Path const & path) -> Result<DirectoryIterator>;


enum class EntryType
{
    file = LFS_TYPE_REG,
    directory = LFS_TYPE_DIR,
};


struct DirectoryInfo
{
    EntryType type;
    Path name;
    std::uint32_t size;

    friend auto operator==(DirectoryInfo const &, DirectoryInfo const &) -> bool = default;
};


// TODO: Move this class to a separate file
class DirectoryIterator
{
public:
    // NOLINTBEGIN(readability-identifier-naming)
    using value_type = Result<DirectoryInfo>;
    using difference_type = int;
    using pointer = Result<DirectoryInfo> *;
    using reference = Result<DirectoryInfo> &;
    using iterator_category = std::input_iterator_tag;
    // NOLINTEND(readability-identifier-naming)

    DirectoryIterator(DirectoryIterator const & other) noexcept;
    DirectoryIterator(DirectoryIterator && other) noexcept;
    auto operator=(DirectoryIterator const & other) noexcept -> DirectoryIterator &;
    auto operator=(DirectoryIterator && other) noexcept -> DirectoryIterator &;
    ~DirectoryIterator();

    friend auto MakeIterator(Path const & path) -> Result<DirectoryIterator>;
    friend auto operator==(DirectoryIterator const & lhs, DirectoryIterator const & rhs) -> bool;

    // Post-increment returns by value → copy → too expensive for us so we don't implement it
    auto operator++() -> DirectoryIterator &;
    [[nodiscard]] auto operator*() const -> Result<DirectoryInfo>;

    [[nodiscard]] auto begin() const -> DirectoryIterator;  // NOLINT(readability-identifier-naming)
    [[nodiscard]] static auto end() -> DirectoryIterator;   // NOLINT(readability-identifier-naming)


private:
    DirectoryIterator() = default;
    auto CopyConstructFrom(DirectoryIterator const * other) noexcept -> void;
    auto ReadNextDirectoryEntry() -> void;
    // If an error occurs, we want to go into the default state like the end iterator but keep the
    // error code
    auto ResetEverythingExceptErrorCode() -> void;

    Path path_ = "";
    lfs_dir_t lfsDirectory_ = {};
    DirectoryInfo directoryInfo_ = {};
    int lfsErrorCode_ = 0;
};


// TODO: Move this class to a separate file
class File
{
public:
    File(File const &) = delete;
    File(File && other) noexcept;
    auto operator=(File const &) -> File & = delete;
    auto operator=(File && other) noexcept -> File &;
    ~File();

    friend auto Open(Path const & path, unsigned int flags) -> Result<File>;

    template<std::size_t extent>
    [[nodiscard]] auto Read(std::span<Byte, extent> data) const -> Result<int>;
    template<std::size_t extent>
    [[nodiscard]] auto Write(std::span<const Byte, extent> data) -> Result<int>;
    [[nodiscard]] auto SeekAbsolute(int offset) -> Result<int>;
    [[nodiscard]] auto SeekRelative(int offset) -> Result<int>;
    [[nodiscard]] auto Size() const -> Result<int>;
    [[nodiscard]] auto Close() const -> Result<void>;
    [[nodiscard]] auto Flush() -> Result<void>;


private:
    // Only allow creation of File class through friend function Open()
    File() = default;
    auto MoveConstructFrom(File * other) noexcept -> void;
    [[nodiscard]] auto CreateLockFile() const noexcept -> Result<void>;
    [[nodiscard]] auto Read(void * buffer, std::size_t size) const -> Result<int>;
    [[nodiscard]] auto Write(void const * buffer, std::size_t size) -> Result<int>;
    [[nodiscard]] auto Seek(int offset, int whence) -> Result<int>;
    [[nodiscard]] auto CloseAndKeepLockFile() const -> Result<void>;

    Path path_ = "";
    Path lockFilePath_ = "";
    unsigned int openFlags_ = 0;
    mutable lfs_file_t lfsFile_ = {};
    mutable bool isOpen_ = false;
    std::array<Byte, lfsCacheSize> buffer_ = {};
    lfs_file_config lfsFileConfig_ = {.buffer = buffer_.data()};
};
}


#include <Sts1CobcSw/FileSystem/LfsWrapper.ipp>  // IWYU pragma: keep
