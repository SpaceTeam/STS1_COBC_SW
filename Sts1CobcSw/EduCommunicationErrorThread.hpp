#pragma once


#include <Sts1CobcSw/Periphery/Edu.hpp>

#include <cstdint>


namespace sts1cobcsw
{
extern std::int32_t eduCommunicationErrorCounter;


auto ResumeEduErrorCommunicationThread() -> void;
}
