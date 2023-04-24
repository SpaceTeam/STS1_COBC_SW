#pragma once


#include <Sts1CobcSw/Periphery/Fram.hpp>


namespace sts1cobcsw::periphery::fram
{
template<typename T>
auto Read(Address address) -> T
{
    auto t = T{};
    Read(address, &t, sizeof(T));
    return t;
}


template<typename T>
auto Write(Address address, T const & t) -> void
{
    Write(address, &t, sizeof(T));
}
}
