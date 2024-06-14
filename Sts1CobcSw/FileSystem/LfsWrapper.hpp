#pragma once


#include <Sts1CobcSw/FileSystem/ErrorsAndResult.hpp>

#include <littlefs/lfs.h>

#include <etl/string.h>

#include <string_view>


namespace sts1cobcsw::fs
{
using Path = etl::string<LFS_NAME_MAX>;


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
    lfs_file_t lfsFile_ = {};
};
}


#include <Sts1CobcSw/FileSystem/LfsWrapper.ipp>  // IWYU pragma: keep