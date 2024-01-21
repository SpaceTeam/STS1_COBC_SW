#pragma once


#include <Sts1CobcSw/Edu/Types.hpp>
#include <Sts1CobcSw/Outcome/Outcome.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>

#include <rodos_no_using_namespace.h>

#include <cstdint>


namespace sts1cobcsw::edu
{
template<typename T>
using Result = outcome_v2::experimental::status_result<T, edu::ErrorCode, RebootPolicy>;

auto Initialize() -> void;
auto TurnOn() -> void;
auto TurnOff() -> void;

// TODO: Why does this return a std::int32_t?
[[nodiscard]] auto StoreProgram(StoreProgramData const & data) -> Result<std::int32_t>;
[[nodiscard]] auto ExecuteProgram(ExecuteProgramData const & data) -> Result<void>;
[[nodiscard]] auto StopProgram() -> Result<void>;
// TODD: Find better name (or maybe even mechanism) for GetStatus
[[nodiscard]] auto GetStatus() -> Result<Status>;
[[nodiscard]] auto ReturnResult(ReturnResultData const & data) -> Result<ResultInfo>;
[[nodiscard]] auto UpdateTime(UpdateTimeData const & data) -> Result<void>;
}
