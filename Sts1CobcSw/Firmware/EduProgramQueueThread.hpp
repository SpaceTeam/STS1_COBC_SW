#pragma once


#include <cstdint>


namespace sts1cobcsw
{
inline constexpr std::uint8_t eduProgramQueueIndexResetValue = 0xFF;


auto ResumeEduProgramQueueThread() -> void;
}
