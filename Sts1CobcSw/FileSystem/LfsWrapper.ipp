#pragma once

#include <Sts1CobcSw/FileSystem/LfsWrapper.hpp>


namespace sts1cobcsw::fs
{
template<typename T>
auto File::Read(T * t) const -> Result<int>
{
    return ReadInternal(t, sizeof(T));
}


template<typename T>
auto File::Write(T const & t) -> Result<int>
{
    return WriteInternal(&t, sizeof(T));
}
}
