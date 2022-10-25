#pragma once

#include <Sts1CobcSw/Serial/Byte.hpp>

#include <rodos_no_using_namespace.h>


// TODO: This could be in Edu.cpp cause AFAICS users don't need to know about this
namespace sts1cobcsw::periphery
{
using sts1cobcsw::serial::operator""_b;


// TODO: Maybe turn this into enums
// CEP high-level command headers (see EDU PDD)
inline constexpr auto storeArchiveId = 0x01_b;    //! Transfer student programs from COBC to EDU
inline constexpr auto executeProgramId = 0x02_b;  //! Execute student program
inline constexpr auto stopProgramId = 0x03_b;     //! Stop student program
inline constexpr auto getStatusId = 0x04_b;       //! Get the student program status
inline constexpr auto returnResultId = 0x05_b;    //! Request student program result
inline constexpr auto updateTimeId = 0x06_b;      //! Update EDU system time
}
