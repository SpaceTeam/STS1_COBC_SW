#pragma once


#include <Sts1CobcSw/FileSystem/LfsMemoryDevice.hpp>
#include <Sts1CobcSw/Outcome/Outcome.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>

#include <littlefs/lfs.h>

#include <etl/string.h>

#include <array>
#include <cstddef>
#include <span>


namespace sts1cobcsw::fs
{
using Path = etl::string<maxPathLength>;

class File;


[[nodiscard]] auto Mount() -> Result<void>;
[[nodiscard]] auto Unmount() -> Result<void>;
[[nodiscard]] auto Open(Path const & path, unsigned int flags) -> Result<File>;


// TODO: Consider moving this class to a separate file
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
