#include <Sts1CobcSw/Periphery/Edu.hpp>
#include <Sts1CobcSw/Periphery/EduStructs.hpp>
#include <Sts1CobcSw/Periphery/Enums.hpp>

#include <rodos_no_using_namespace.h>

#include <cstdint>


namespace sts1cobcsw
{
class UpdateTimeTest : public RODOS::StaticThread<>
{
    void run() override
    {
        auto edu = periphery::Edu();
        constexpr std::int32_t timestamp = 0x12345678;

        RODOS::PRINTF("\nSending command 'Update Time' with\n");
        RODOS::PRINTF("- timestamp = %08x\n", static_cast<unsigned int>(timestamp));

        auto errorCode = edu.UpdateTime({.timestamp = timestamp});
        RODOS::PRINTF("\nReturned error code: %d", static_cast<int>(errorCode));
    }
};


auto const updateTimeTest = UpdateTimeTest();
}