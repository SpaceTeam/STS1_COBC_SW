#pragma once


#include <Sts1CobcSw/Edu/Enums.hpp>
#include <Sts1CobcSw/Edu/Structs.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>

#include <rodos_no_using_namespace.h>

#include <cstdint>


namespace sts1cobcsw::edu
{
auto Initialize() -> void;
auto TurnOn() -> void;
auto TurnOff() -> void;

// TODO: Why does this return a std::int32_t?
[[nodiscard]] auto StoreArchive(StoreArchiveData const & data) -> std::int32_t;
[[nodiscard]] auto ExecuteProgram(ExecuteProgramData const & data) -> ErrorCode;
[[nodiscard]] auto StopProgram() -> ErrorCode;
// TODD: Find better name (or maybe even mechanism) for GetStatus
[[nodiscard]] auto GetStatus() -> Status;
[[nodiscard]] auto ReturnResult() -> ResultInfo;
[[nodiscard]] auto UpdateTime(UpdateTimeData const & data) -> ErrorCode;
}
