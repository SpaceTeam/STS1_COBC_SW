#include <Sts1CobcSw/Flash/FlashMock.hpp>
#include <Sts1CobcSw/Outcome/Outcome.hpp>


namespace sts1cobcsw::flash
{
auto doInitialize = empty::DoInitialize;
auto doReadJedecId = empty::DoReadJedecId;
auto doReadStatusRegister = empty::DoReadStatusRegister;

auto doReadPage = empty::DoReadPage;
auto doProgramPage = empty::DoProgramPage;
auto doEraseSector = empty::DoEraseSector;
auto doWaitWhileBusy = empty::DoWaitWhileBusy;
auto doActualBaudRate = empty::DoActualBaudRate;


// --- Mocked functions ---

auto Initialize() -> void
{
    return doInitialize();
}


auto ReadJedecId() -> JedecId
{
    return doReadJedecId();
}


auto ReadStatusRegister(std::int8_t registerNo) -> Byte
{
    return doReadStatusRegister(registerNo);
}


auto ReadPage(std::uint32_t address) -> Page
{
    return doReadPage(address);
}


auto ProgramPage(std::uint32_t address, PageSpan data) -> void
{
    return doProgramPage(address, data);
}


auto EraseSector(std::uint32_t address) -> void
{
    return doEraseSector(address);
}


auto WaitWhileBusy(Duration timeout) -> Result<void>
{
    return doWaitWhileBusy(timeout);
}


auto ActualBaudRate() -> std::int32_t
{
    return doActualBaudRate();
}


// --- Set functions ---

auto SetDoInitialize(void (*doInitializeFunction)()) -> void
{
    doInitialize = doInitializeFunction;
}


auto SetDoReadJedecId(JedecId (*doReadJedecIdFunction)()) -> void
{
    doReadJedecId = doReadJedecIdFunction;
}


auto SetDoReadStatusRegister(Byte (*doReadStatusRegisterFunction)(std::int8_t registerNo)) -> void
{
    doReadStatusRegister = doReadStatusRegisterFunction;
}


auto SetDoReadPage(Page (*doReadPageFunction)(std::uint32_t address)) -> void
{
    doReadPage = doReadPageFunction;
}


auto SetDoProgramPage(void (*doProgramPageFunction)(std::uint32_t address, PageSpan data)) -> void
{
    doProgramPage = doProgramPageFunction;
}


auto SetDoEraseSector(void (*doEraseSectorFunction)(std::uint32_t address)) -> void
{
    doEraseSector = doEraseSectorFunction;
}


auto SetDoWaitWhileBusy(Result<void> (*doWaitWhileBusyFunction)(Duration timeout)) -> void
{
    doWaitWhileBusy = doWaitWhileBusyFunction;
}


auto SetDoActualBaudRate(std::int32_t (*doActualBaudRateFunction)()) -> void
{
    doActualBaudRate = doActualBaudRateFunction;
}


// --- Predefined do functions ---

namespace empty
{
auto SetAllDoFunctions() -> void
{
    SetDoInitialize(DoInitialize);
    SetDoReadJedecId(DoReadJedecId);
    SetDoReadStatusRegister(DoReadStatusRegister);

    SetDoReadPage(DoReadPage);
    SetDoProgramPage(DoProgramPage);
    SetDoEraseSector(DoEraseSector);
    SetDoWaitWhileBusy(DoWaitWhileBusy);
    SetDoActualBaudRate(DoActualBaudRate);
}


auto DoInitialize() -> void
{
}


auto DoReadJedecId() -> JedecId
{
    return JedecId{};
}


auto DoReadStatusRegister([[maybe_unused]] std::int8_t registerNo) -> Byte
{
    return Byte{0};
}


auto DoReadPage([[maybe_unused]] std::uint32_t address) -> Page
{
    return Page{};
}


auto DoProgramPage([[maybe_unused]] std::uint32_t address, [[maybe_unused]] PageSpan data) -> void
{
}


auto DoEraseSector([[maybe_unused]] std::uint32_t address) -> void
{
}


auto DoWaitWhileBusy([[maybe_unused]] Duration timeout) -> Result<void>
{
    return outcome_v2::success();
}


auto DoActualBaudRate() -> std::int32_t
{
    return 0;
}
}
}
