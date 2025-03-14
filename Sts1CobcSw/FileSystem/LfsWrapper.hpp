#pragma once


#include <Sts1CobcSw/FileSystem/ErrorsAndResult.hpp>
#include <Sts1CobcSw/FileSystem/LfsMemoryDevice.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>

#include <littlefs/lfs.h>

#include <etl/string.h>
#include <etl/vector.h>

#include <array>
#include <cstddef>
#include <span>


namespace sts1cobcsw::fs
{
constexpr size_t lsOutputSize = 256;  // TODO: find ls max length and max files per dir
constexpr size_t lsMaxFiles = 10;

using Path = etl::string<maxPathLength>;
using DirectoryOutput = etl::vector<etl::string<lsOutputSize>, lsMaxFiles>;
// TODO: Create our own struct using Path and an enum class for the entry type
using DirectoryInfo = lfs_info;


class File;
class DirectoryIterator;


[[nodiscard]] auto Mount() -> Result<void>;
[[nodiscard]] auto Unmount() -> Result<void>;
[[nodiscard]] auto Open(Path const & path, unsigned int flags) -> Result<File>;
[[nodiscard]] auto CreateDirectory(Path const & path) -> Result<void>;
[[nodiscard]] auto Remove(Path const & path) -> Result<void>;
[[nodiscard]] auto ForceRemove(Path const & path) -> Result<void>;
[[nodiscard]] auto Ls(Path const & path) -> Result<DirectoryOutput>;
[[nodiscard]] auto MakeIterator(Path const & path) -> Result<DirectoryIterator>;


// TODO: Move this class to a separate file
class DirectoryIterator
{
public:
    // TODO: Enable and implement copy constructor and copy assignment operator
    DirectoryIterator(DirectoryIterator const &) = delete;
    DirectoryIterator(DirectoryIterator && other) noexcept;
    auto operator=(DirectoryIterator const &) -> DirectoryIterator & = delete;
    auto operator=(DirectoryIterator && other) noexcept -> DirectoryIterator &;
    ~DirectoryIterator();

    friend auto MakeIterator(Path const & path) -> Result<DirectoryIterator>;

    auto operator++() -> DirectoryIterator &;                        // Pre-increment
    auto operator*() const -> Result<DirectoryInfo>;                 // Dereference
    auto operator==(DirectoryIterator const & other) const -> bool;  // Equality check
    auto operator!=(DirectoryIterator const & other) const -> bool;  // Equality check

    // NOLINTBEGIN(readability*)
    [[nodiscard]] auto begin() const -> DirectoryIterator;
    [[nodiscard]] auto end() const -> DirectoryIterator;
    // NOLINTEND(readability*)


private:
    DirectoryIterator() = default;
    auto MoveConstructFrom(DirectoryIterator * other) noexcept -> void;
    auto ReadNextDirectoryEntry() -> void;

    Path path_ = "";
    lfs_dir_t lfsDirectory_ = {};
    bool isOpen_ = false;
    bool endIsReached_ = false;
    lfs_info lfsInfo_ = {};
    int lfsFileErrorCode_ = 0;
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
