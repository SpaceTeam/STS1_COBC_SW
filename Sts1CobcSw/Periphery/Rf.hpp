#pragma once


namespace sts1cobcsw::periphery::rf
{
// Must be called once in a thread's init() function
auto Initialize() -> void;

auto PartInfoIsCorrect() -> bool;

auto GetPartInfo() -> std::uint16_t;

auto Morse() -> void;

auto ClearInterrupts() -> void;
}