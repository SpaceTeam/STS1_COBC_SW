#pragma once


#include <Sts1CobcSw/Periphery/Flash.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Vocabulary/TimeTypes.hpp>

#include <cstdint>


namespace sts1cobcsw::flash
{
auto SetDoInitialize(void (*doInitializeFunction)()) -> void;
auto SetDoReadJedecId(JedecId (*doReadJedecIdFunction)()) -> void;
auto SetDoReadStatusRegister(Byte (*doReadStatusRegisterFunction)(std::int8_t registerNo)) -> void;

auto SetDoReadPage(Page (*doReadPageFunction)(std::uint32_t address)) -> void;
auto SetDoProgramPage(void (*doProgramPageFunction)(std::uint32_t address, PageSpan data)) -> void;
auto SetDoEraseSector(void (*doEraseSectorFunction)(std::uint32_t address)) -> void;
auto SetDoWaitWhileBusyFunction(Result<void> (*doWaitWhileBusy)(Duration timeout)) -> void;
auto SetDoActualBaudRate(std::int32_t (*doActualBaudRateFunction)()) -> void;

namespace empty
{
auto SetAllDoFunctions() -> void;

auto DoInitialize() -> void;
auto DoReadJedecId() -> JedecId;
auto DoReadStatusRegister(std::int8_t registerNo) -> Byte;

auto DoReadPage(std::uint32_t address) -> Page;
auto DoProgramPage(std::uint32_t address, PageSpan data) -> void;
auto DoEraseSector(std::uint32_t address) -> void;
auto DoWaitWhileBusy(Duration timeout) -> Result<void>;
auto DoActualBaudRate() -> std::int32_t;
}
}
