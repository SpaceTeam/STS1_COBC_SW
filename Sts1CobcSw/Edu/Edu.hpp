#pragma once


#include <Sts1CobcSw/Edu/Types.hpp>
#include <Sts1CobcSw/FileSystem/FileSystem.hpp>
#include <Sts1CobcSw/Hal/GpioPin.hpp>
#include <Sts1CobcSw/Outcome/Outcome.hpp>
#include <Sts1CobcSw/Vocabulary/Ids.hpp>


namespace sts1cobcsw::edu
{
inline auto const programsDirectory = fs::Path("/programs");
inline auto const resultsDirectory = fs::Path("/results");

extern hal::GpioPin updateGpioPin;
extern hal::GpioPin dosiEnableGpioPin;


auto Initialize() -> void;
auto TurnOn() -> void;
auto TurnOff() -> void;

[[nodiscard]] auto StoreProgram(StoreProgramData const & data) -> Result<void>;
[[nodiscard]] auto ExecuteProgram(ExecuteProgramData const & data) -> Result<void>;
[[nodiscard]] auto StopProgram() -> Result<void>;
// TODO: Find better name (or maybe even mechanism) for GetStatus
[[nodiscard]] auto GetStatus() -> Result<Status>;
[[nodiscard]] auto ReturnResult(ReturnResultData const & data) -> Result<void>;
[[nodiscard]] auto UpdateTime(UpdateTimeData const & data) -> Result<void>;

[[nodiscard]] auto GetProgramId(fs::Path const & filename) -> Result<ProgramId>;
[[nodiscard]] auto ProgramsAreAvailableOnCobc() -> bool;
}
