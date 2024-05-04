#pragma once


#include <Sts1CobcSw/FileSystem/ErrorsAndResult.hpp>


namespace sts1cobcsw::fs
{

[[nodiscard]] auto Mount() -> Result<void>;

class File
{
public:
    // only allow creation of File class throu open friend function
    File() = delete;
    File(File const &) = delete;
    File(File &&) = default;

    ~File();

    [[nodiscard]] auto close() -> Result<void>;
    template<typename T>
    auto read(T * t) -> Result<int>;
    template<typename T>
    auto write(T const & t) -> Result<int>;
    auto FileSize() -> Result<int>;

    friend auto open(char const * path, int flag) -> Result<File>;

private:
    File(lfs_file_t & lfsFile);

    lfs_file_t & m_lfsFile;
};

}
