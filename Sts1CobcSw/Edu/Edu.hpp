#pragma once


#include <Sts1CobcSw/Edu/Enums.hpp>
#include <Sts1CobcSw/Edu/Structs.hpp>
#include <Sts1CobcSw/Hal/GpioPin.hpp>
#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>

#include <rodos_no_using_namespace.h>

#include <cstdint>
#include <span>


namespace sts1cobcsw::edu
{


// TODO: Think about const-correctness and whether to make uart_ mutable or not
//
// TODO: There is no reason for this to be a class (there is no class invariant), so this being a
// class just unnecessarily exposes the private members and functions to the user which makes
// mocking harder.

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
