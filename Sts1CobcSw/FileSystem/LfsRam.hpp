#pragma once

#include <vector>

#include "Sts1CobcSw/Serial/Byte.hpp"

namespace sts1cobcsw::fs
{
extern std::vector<Byte> memory;
void SimulateFailOnNextWrite();
}