#pragma once


#include <Sts1CobcSw/FileSystem/ErrorsAndResult.hpp>
#include <Sts1CobcSw/FileSystem/LfsStorageDevice.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>

#include <littlefs/lfs.h>

#include <etl/string.h>

#include <array>
#include <string_view>


namespace sts1cobcsw::fs
{
// TODO: Consider reducing this to a bit more than 20 = strlen("/programs/65536.zip") to save RAM
using Path = etl::string<LFS_NAME_MAX>;


// TODO: Get rid of this global variable or at least hide it in an internal namespace
extern lfs_t lfs;


class File;

[[nodiscard]] auto Mount() -> Result<void>;
[[nodiscard]] auto Unmount() -> Result<void>;
[[nodiscard]] auto Open(std::string_view path, int flags) -> Result<File>;


class File
{
public:
    File(File const &) = delete;
    File(File && other) noexcept;
    auto operator=(File const &) -> File & = delete;
    auto operator=(File && other) noexcept -> File &;
    ~File();

    // TODO: Read() and Write() should be implemented like ReadFrom() and WriteTo() in Fram.hpp,
    // including forwarding to functions in an internal namespace. This way we can move lfs back
    // into the .cpp file.
    template<typename T>
    [[nodiscard]] auto Read(T * t) -> Result<int>;
    template<typename T>
    [[nodiscard]] auto Write(T const & t) -> Result<int>;
    [[nodiscard]] auto Size() -> Result<int>;
    [[nodiscard]] auto Close() -> Result<void>;

    friend auto Open(std::string_view path, int flags) -> Result<File>;

private:
    // Only allow creation of File class through friend function Open()
    File() = default;

    Path path_ = "";
    int openFlags_ = 0;
    bool isOpen_ = false;
    std::array<Byte, lfsCacheSize> buffer_ = {};
    lfs_file_t lfsFile_ = {};
    lfs_file_config lfsFileConfig_ = {.buffer = buffer_.data()};
};
}


#include <Sts1CobcSw/FileSystem/LfsWrapper.ipp>  // IWYU pragma: keep
