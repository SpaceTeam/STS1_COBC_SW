#pragma once


#include <Sts1CobcSw/Serial/Byte.hpp>

#include <span>


namespace sts1cobcsw
{
auto Scramble(std::span<Byte> data) -> void;
auto Unscramble(std::span<Byte> data) -> void;
}
