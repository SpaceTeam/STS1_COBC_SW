#pragma once


// TODO: This file should not exist. Turning the EDU on or off should be part of the public EDU API.
// BuildQueue should probably be in EduProgramQueue and UpdateUtcOffset() could also be there or in
// Utility/Time.hpp maybe.
namespace sts1cobcsw
{
void TurnEduOn();
void TurnEduOff();
void UpdateUtcOffset();
}
