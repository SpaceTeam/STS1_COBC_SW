#pragma once


namespace sts1cobcsw::periphery::rf
{
// Must be called once in a thread's init() function
auto Initialize() -> void;

auto PartInfoIsCorrect() -> bool;

auto Morse() -> void;

// TODO: implement
// auto SetProperty() -> void;
}