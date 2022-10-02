#pragma once

#include <type_safe/types.hpp>

#include <etl/string.h>


namespace sts1cobcsw
{
using type_safe::operator""_usize;

constexpr auto commandSize = 3_usize;

void TurnEduOn();
void TurnEduOff();
void UpdateUtcOffset();
}
