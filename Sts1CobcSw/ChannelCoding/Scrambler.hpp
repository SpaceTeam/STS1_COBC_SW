#pragma once

#include <cstdint>
#include <span>

#include "../Serial/Byte.hpp"

std::uint8_t constexpr fieldSize = 255;

auto Unscramble(std::span<sts1cobcsw::Byte> data) -> void;
auto Scramble(std::span<sts1cobcsw::Byte> data) -> void;
