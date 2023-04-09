#include <Sts1CobcSw/Periphery/Flash.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>

#include <Tests/HardwareTests/Utility.hpp>

#include <rodos_no_using_namespace.h>

#include <algorithm>
#include <cstdint>
#include <string_view>


namespace sts1cobcsw
{
using RODOS::PRINTF;
using serial::operator""_b;


constexpr std::size_t stackSize = 5'000;
std::int32_t errorCode = 0;


auto Print(periphery::flash::Page const & page) -> void;


class FlashTest : public RODOS::StaticThread<stackSize>
{
public:
    FlashTest() : StaticThread("FlashTest")
    {
    }

private:
    void init() override
    {
        errorCode = periphery::flash::Initialize();
    }


    void run() override
    {
        PRINTF("\nFlash Test\n\n");

        PRINTF("Initialize(): %i == 0\n", static_cast<int>(errorCode));
        Check(errorCode == 0);

        PRINTF("\n");
        auto jedecId = periphery::flash::ReadJedecId();
        PRINTF("Manufacturer ID: 0x%02x == 0xEF\n",
               static_cast<unsigned int>(jedecId.manufacturerId));
        Check(jedecId.manufacturerId == 0xEF);
        PRINTF("Device ID: 0x%04x == 0x4021\n", static_cast<unsigned int>(jedecId.deviceId));
        Check(jedecId.deviceId == 0x4021);

        PRINTF("\n");
        auto statusRegister = periphery::flash::ReadStatusRegister(1);
        PRINTF("Status Register 1: 0x%02x == 0x00\n", static_cast<unsigned int>(statusRegister));
        Check(statusRegister == 0x00_b);

        statusRegister = periphery::flash::ReadStatusRegister(2);
        PRINTF("Status Register 2: 0x%02x == 0x02\n", static_cast<unsigned int>(statusRegister));
        Check(statusRegister == 0x02_b);

        statusRegister = periphery::flash::ReadStatusRegister(3);
        PRINTF("Status Register 3: 0x%02x == 0x41\n", static_cast<unsigned int>(statusRegister));
        Check(statusRegister == 0x41_b);

        std::uint32_t pageAddress = 0x00'01'00'00;

        PRINTF("\n");
        PRINTF("Reading page at address 0x%08x:\n", static_cast<unsigned int>(pageAddress));
        auto page = periphery::flash::ReadPage(pageAddress);
        Print(page);

        PRINTF("\n");
        std::fill(begin(page), end(page), 0x00_b);
        PRINTF("Programming page at address 0x%08x:\n", static_cast<unsigned int>(pageAddress));
        Print(page);
        periphery::flash::ProgramPage(pageAddress, page);

        auto begin = RODOS::NOW();
        periphery::flash::WaitWhileBusy();
        auto end = RODOS::NOW();
        PRINTF("Programming page took %d ms\n",
               static_cast<int>((end - begin) / RODOS::MILLISECONDS));

        PRINTF("\n");
        PRINTF("Reading page at address 0x%08x:\n", static_cast<unsigned int>(pageAddress));
        page = periphery::flash::ReadPage(pageAddress);
        Print(page);

        PRINTF("\n");
        // constexpr auto sectorAddress = 0x00'00'00'00;
        PRINTF("Erasing sector containing address 0x%08x:\n",
               static_cast<unsigned int>(pageAddress));
        periphery::flash::EraseSector(pageAddress);

        begin = RODOS::NOW();
        periphery::flash::WaitWhileBusy();
        end = RODOS::NOW();
        PRINTF("Erasing sector took %d ms\n",
               static_cast<int>((end - begin) / RODOS::MILLISECONDS));

        PRINTF("\n");
        PRINTF("Reading page at address 0x%08x:\n", static_cast<unsigned int>(pageAddress));
        page = periphery::flash::ReadPage(pageAddress);
        Print(page);
    }
} flashTest;


auto Print(periphery::flash::Page const & page) -> void
{
    constexpr auto nRows = 16;
    auto iRow = 0;
    for(auto x : page)
    {
        PRINTF(" 0x%02x", static_cast<unsigned int>(x));
        iRow++;
        if(iRow == nRows)
        {
            PRINTF("\n");
            iRow = 0;
        }
    }
}
}