#include <Tests/HardwareTests/Utility.hpp>

#include <Sts1CobcSw/Periphery/Flash.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Utility/RodosTime.hpp>
#include <Sts1CobcSw/Utility/TimeTypes.hpp>

#include <strong_type/affine_point.hpp>
#include <strong_type/difference.hpp>
#include <strong_type/type.hpp>

#include <rodos_no_using_namespace.h>

#include <algorithm>
#include <cinttypes>
#include <cstddef>
#include <cstdint>


namespace sts1cobcsw
{
using RODOS::PRINTF;


constexpr std::size_t stackSize = 5'000;


auto Print(flash::Page const & page) -> void;


class FlashTest : public RODOS::StaticThread<stackSize>
{
public:
    FlashTest() : StaticThread("FlashTest")
    {
    }


private:
    void init() override
    {
    }


    void run() override
    {
        PRINTF("\nFlash test\n\n");

        PRINTF("\n");
        flash::Initialize();
        auto actualBaudRate = flash::ActualBaudRate();
        PRINTF("Actual baud rate: %" PRIi32 "\n", actualBaudRate);

        PRINTF("\n");
        auto jedecId = flash::ReadJedecId();
        PRINTF("Manufacturer ID: 0x%02x == 0x%02x\n",
               static_cast<unsigned int>(jedecId.manufacturerId),
               static_cast<unsigned int>(flash::correctJedecId.manufacturerId));
        Check(jedecId.manufacturerId == flash::correctJedecId.manufacturerId);
        PRINTF("Device ID: 0x%04x == 0x%04x\n",
               static_cast<unsigned int>(jedecId.deviceId),
               static_cast<unsigned int>(flash::correctJedecId.deviceId));
        Check(jedecId.deviceId == flash::correctJedecId.deviceId);

        PRINTF("\n");
        auto statusRegister = flash::ReadStatusRegister(1);
        PRINTF("Status register 1: 0x%02x == 0x00\n", static_cast<unsigned int>(statusRegister));
        Check(statusRegister == 0x00_b);

        statusRegister = flash::ReadStatusRegister(2);
        PRINTF("Status register 2: 0x%02x == 0x02\n", static_cast<unsigned int>(statusRegister));
        Check(statusRegister == 0x02_b);

        statusRegister = flash::ReadStatusRegister(3);
        PRINTF("Status register 3: 0x%02x == 0x41\n", static_cast<unsigned int>(statusRegister));
        Check(statusRegister == 0x41_b);

        std::uint32_t const pageAddress = 0x00'01'00'00U;

        PRINTF("\n");
        PRINTF("Reading page at address 0x%08x:\n", static_cast<unsigned int>(pageAddress));
        auto page = flash::ReadPage(pageAddress);
        Print(page);

        PRINTF("\n");
        std::fill(page.begin(), page.end(), 0x00_b);
        PRINTF("Programming page at address 0x%08x:\n", static_cast<unsigned int>(pageAddress));
        Print(page);
        auto begin = CurrentRodosTime();
        flash::ProgramPage(pageAddress, page);
        auto endPage = CurrentRodosTime();

        auto const programPageTimeout = 10 * ms;
        auto waitWhileBusyResult = flash::WaitWhileBusy(programPageTimeout);
        auto end = CurrentRodosTime();
        PRINTF("ProgrammPage took %d us\n", static_cast<int>((endPage - begin) / us));
        if(waitWhileBusyResult.has_error())
        {
            PRINTF("WaitWhileBusy failed because it didn't finish in %d us\n",
                   static_cast<int>(programPageTimeout / us));
        }
        else
        {
            PRINTF("WaitWhileBusy took %d us\n", static_cast<int>((end - endPage) / us));
        }

        PRINTF("\n");
        PRINTF("Reading page at address 0x%08x:\n", static_cast<unsigned int>(pageAddress));
        page = flash::ReadPage(pageAddress);
        Print(page);

        PRINTF("\n");
        PRINTF("Erasing sector containing address 0x%08x:\n",
               static_cast<unsigned int>(pageAddress));
        flash::EraseSector(pageAddress);

        auto const eraseSectorTimeout = 500 * ms;
        begin = CurrentRodosTime();
        waitWhileBusyResult = flash::WaitWhileBusy(eraseSectorTimeout);
        end = CurrentRodosTime();
        if(waitWhileBusyResult.has_error())
        {
            PRINTF("WaitWhileBusy failed because it didn't finish in %d us\n",
                   static_cast<int>(eraseSectorTimeout / us));
        }
        else
        {
            PRINTF("WaitWhileBusy took %d us\n", static_cast<int>((end - begin) / us));
        }

        PRINTF("\n");
        PRINTF("Reading page at address 0x%08x:\n", static_cast<unsigned int>(pageAddress));
        page = flash::ReadPage(pageAddress);
        Print(page);
    }
} flashTest;


auto Print(flash::Page const & page) -> void
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
