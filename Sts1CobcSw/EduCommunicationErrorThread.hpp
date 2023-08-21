#pragma once


#include <cstdint>


namespace sts1cobcsw
{
extern std::int32_t eduCommunicationErrorCounter;


auto ResumeEduCommunicationErrorThread() -> void;
}
