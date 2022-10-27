#pragma once

#include <type_safe/types.hpp>


// TODO: This file should not exist. Turning the EDU on or off should be part of the public EDU API.
// BuildQueue should probably be in EduProgramQueue and UpdateUtcOffset() could also be there or in
// Utility/Time.hpp maybe.
namespace sts1cobcsw
{
using type_safe::operator""_usize;

constexpr auto commandSize = 30_usize;

void TurnEduOn();
void TurnEduOff();
void UpdateUtcOffset();
}
