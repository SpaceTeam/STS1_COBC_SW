#pragma once


#include <Sts1CobcSw/Serial/Byte.hpp>

#include <span>


namespace sts1cobcsw
{
namespace tc
{
auto Scramble(std::span<Byte> data) -> void;
auto Unscramble(std::span<Byte> data) -> void;
}

namespace tm
{
auto Scramble(std::span<Byte> data) -> void;
auto Unscramble(std::span<Byte> data) -> void;
}
}
