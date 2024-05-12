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
    File(File const &) = delete;
    File(File &&) = default;
    auto operator=(File const &) -> File & = delete;
    auto operator=(File &&) -> File & = default;
    ~File();

    template<typename T>
    [[nodiscard]] auto Read(T * t) -> Result<int>;
    template<typename T>
    [[nodiscard]] auto Write(T const & t) -> Result<int>;
    [[nodiscard]] auto Size() -> Result<int>;
    [[nodiscard]] auto Close() -> Result<void>;

    friend auto Open(std::string_view path, int flag) -> Result<File>;

private:
    // Only allow creation of File class through friend function Open()
    File() = default;

    lfs_file_t lfsFile_;
};
}
