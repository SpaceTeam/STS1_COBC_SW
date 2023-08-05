#pragma once


#include <Sts1CobcSw/Periphery/Edu.hpp>


namespace sts1cobcsw
{
// TODO: This should be in Edu.hpp
extern periphery::Edu edu;


auto ResumeEduProgramQueueThread() -> void;
}
