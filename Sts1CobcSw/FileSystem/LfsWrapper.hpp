#pragma once


#include <Sts1CobcSw/FileSystem/ErrorsAndResult.hpp>
#include <Sts1CobcSw/FileSystem/LfsMemoryDevice.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>

#include <littlefs/lfs.h>

#include <etl/string.h>

#include <array>
#include <cstddef>
#include <string_view>


namespace sts1cobcsw::fs
{
// TODO: Consider reducing this to a bit more than 20 = strlen("/programs/65536.zip") to save RAM
using Path = etl::string<LFS_NAME_MAX>;


class File;

[[nodiscard]] auto Mount() -> Result<void>;
[[nodiscard]] auto Unmount() -> Result<void>;
[[nodiscard]] auto Open(std::string_view path, unsigned int flags) -> Result<File>;


// FIXME: Make File const-correct (only Write() should be non-const)
// TODO: Consider moving this class to a separate file
class File
{
public:
    File(File const &) = delete;
    File(File && other) noexcept;
    auto operator=(File const &) -> File & = delete;
    auto operator=(File && other) noexcept -> File &;
    ~File();

    friend auto Open(std::string_view path, unsigned int flags) -> Result<File>;

    template<typename T>
    [[nodiscard]] auto Read(T * t) const -> Result<int>;
    template<typename T>
    [[nodiscard]] auto Write(T const & t) -> Result<int>;
    [[nodiscard]] auto Size() const -> Result<int>;
    [[nodiscard]] auto Close() -> Result<void>;


private:
    // Only allow creation of File class through friend function Open()
    File() = default;
    auto MoveConstructFrom(File * other) noexcept -> void;
    [[nodiscard]] auto WriteInternal(void const * buffer, std::size_t size) -> Result<int>;
    [[nodiscard]] auto ReadInternal(void * buffer, std::size_t size) const -> Result<int>;

    Path path_ = "";
    unsigned int openFlags_ = 0;
    mutable lfs_file_t lfsFile_ = {};
    bool isOpen_ = false;
    std::array<Byte, lfsCacheSize> buffer_ = {};
    lfs_file_config lfsFileConfig_ = {.buffer = buffer_.data()};
};
}


#include <Sts1CobcSw/FileSystem/LfsWrapper.ipp>  // IWYU pragma: keep
