#pragma once


#include <Sts1CobcSw/Periphery/Edu.hpp>


namespace sts1cobcsw
{
// TODO: Maybe this should be in EduPowerManagement thread
extern periphery::Edu edu;
extern int32_t eduCommunicationErrorCounter;

auto ResumeEduErrorCommunicationThread() -> void;
}
