#include <Sts1CobcSw/Periphery/Flash.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>

#include <rodos_no_using_namespace.h>

#include <cstdint>
#include <string_view>


namespace sts1cobcsw
{
using RODOS::PRINTF;
using serial::operator""_b;


auto Check(bool condition,
           std::string_view failMessage = " -> Failed\n",
           std::string_view successMessage = " -> Passed\n")
{
    if(condition)
    {
        PRINTF("%s", data(successMessage));
    }
    else
    {
        PRINTF("%s", data(failMessage));
    }
}


constexpr std::size_t stackSize = 5'000;
std::int32_t errorCode = 0;


class FlashTest : public RODOS::StaticThread<stackSize>
{
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
        PRINTF("Status Register 3: 0x%02x == 0x40\n", static_cast<unsigned int>(statusRegister));
        Check(statusRegister == 0x40_b);
    }
};


const auto flashTest = FlashTest();
}