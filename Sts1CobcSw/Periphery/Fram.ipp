#pragma once


#include <Sts1CobcSw/Periphery/Fram.hpp>


namespace sts1cobcsw::fram
{
template<typename T>
auto Read(Address address) -> T
{
    auto t = T{};
    ReadFrom(address, &t, sizeof(T));
    return t;
}


template<typename T>
auto WriteTo(Address address, T const & t) -> void
{
    WriteTo(address, &t, sizeof(T));
}
}
