#pragma once

#include <Sts1CobcSw/FileSystem/ErrorsAndResult.hpp>

#include <littlefs/lfs.h>

#include <string_view>


namespace sts1cobcsw::fs
{
class File;


[[nodiscard]] auto Mount() -> Result<void>;


[[nodiscard]] auto Open(std::string_view path, int flag) -> Result<File>;


class File
{
public:
    // only allow creation of File class throu Open friend function
    File(File const &) = delete;
    File(File &&) = default;
    auto operator=(File const &) -> File & = delete;
    auto operator=(File &&) -> File & = delete;
    ~File();

    [[nodiscard]] auto Close() -> Result<void>;
    template<typename T>
    [[nodiscard]] auto Read(T * t) -> Result<int>;
    template<typename T>
    [[nodiscard]] auto Write(T const & t) -> Result<int>;
    [[nodiscard]] auto Size() -> Result<int>;

    friend auto Open(std::string_view path, int flag) -> Result<File>;

private:
    File() = default;

    lfs_file_t lfsFile_;
};
}
