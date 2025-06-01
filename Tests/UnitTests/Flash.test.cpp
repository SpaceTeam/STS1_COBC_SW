#include <Sts1CobcSw/Flash/Flash.hpp>

#include <Tests/CatchRodos/TestMacros.hpp>

#include <Sts1CobcSw/Outcome/Outcome.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Vocabulary/Time.hpp>

#include <strong_type/difference.hpp>

#include <array>


namespace flash = sts1cobcsw::flash;
using sts1cobcsw::ms;
using sts1cobcsw::operator""_b;


TEST_CASE("Flash")
{
    flash::Initialize();
    auto actualBaudRate = flash::ActualBaudRate();
    CHECK(actualBaudRate == 48'000'000);

    auto jedecId = flash::ReadJedecId();
    CHECK(jedecId.manufacturerId == flash::correctJedecId.manufacturerId);
    CHECK(jedecId.deviceId == flash::correctJedecId.deviceId);

    auto statusRegister1 = flash::ReadStatusRegister(1);
    CHECK(statusRegister1 == 0x00_b);
    auto statusRegister2 = flash::ReadStatusRegister(2);
    CHECK(statusRegister2 == 0x02_b);
    auto statusRegister3 = flash::ReadStatusRegister(3);
    CHECK(statusRegister3 == 0x40_b);

    // TODO: Test programming and erasing all pages and sectors. This will take a long time, so
    // maybe it's better to do this in a separate WholeFlash.test.cpp.
    static constexpr auto address = 0x0001'0000U;

    auto page = flash::Page{};
    flash::ProgramPage(address, page);
    auto waitWhileBusyResult = flash::WaitWhileBusy(5 * ms);
    CHECK(waitWhileBusyResult.has_error() == false);
    page = flash::ReadPage(address);
    CHECK(page == flash::Page{});

    flash::EraseSector(address);
    waitWhileBusyResult = flash::WaitWhileBusy(500 * ms);
    CHECK(waitWhileBusyResult.has_error() == false);
    page = flash::ReadPage(address);
    static constexpr auto erasedPage = []()
    {
        auto p = flash::Page();
        p.fill(0xFF_b);
        return p;
    }();
    CHECK(page == erasedPage);
}
