#pragma once


#include <Sts1CobcSw/FileSystem/File.hpp>


namespace sts1cobcsw::fs
{
template<std::size_t extent>
auto File::Read(std::span<Byte, extent> data) const -> Result<int>
{
    return Read(data.data(), data.size());
}


template<std::size_t extent>
auto File::Write(std::span<const Byte, extent> data) -> Result<int>
{
    return Write(data.data(), data.size());
}
}
