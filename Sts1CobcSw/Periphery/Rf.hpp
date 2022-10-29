#pragma once


#include <cstddef>
#include <cstdint>
#include <span>


namespace sts1cobcsw::periphery::rf
{
// Must be called once in a thread's init() function
auto Initialize() -> void;
auto Morse() -> void;

template<std::size_t size>
auto Send(std::span<std::uint8_t, size> data) -> void;
}