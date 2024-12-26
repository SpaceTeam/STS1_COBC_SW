#pragma once


#include <Sts1CobcSw/Serial/Byte.hpp>

#include <vector>


namespace sts1cobcsw::fs
{
extern std::vector<Byte> memory;


auto SetProgramFinishedHandler(void (*handler)()) -> void;
}
