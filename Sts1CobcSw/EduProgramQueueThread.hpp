#pragma once


#include <Sts1CobcSw/Periphery/Edu.hpp>


namespace sts1cobcsw
{
extern periphery::Edu edu;


auto ResumeEduQueueThread() -> void;
}