#include <Sts1CobcSw/Periphery/Edu.hpp>
#include <Sts1CobcSw/Periphery/EduEnums.hpp>
#include <Sts1CobcSw/Periphery/EduStructs.hpp>

#include <rodos_no_using_namespace.h>

#include <cstdint>


namespace sts1cobcsw
{
auto edu = periphery::Edu();


class UpdateTimeTest : public RODOS::StaticThread<>
{
    void init() override
    {
        edu.Initialize();
    }


    void run() override
    {
        constexpr std::int32_t timestamp = 0x12345678;

        RODOS::PRINTF("\nSending command 'Update Time' with\n");
        RODOS::PRINTF("- timestamp = 0x%08x\n", static_cast<unsigned int>(timestamp));

        auto errorCode = edu.UpdateTime({.timestamp = timestamp});
        RODOS::PRINTF("\nReturned error code: %d", static_cast<int>(errorCode));
    }
};


auto const updateTimeTest = UpdateTimeTest();
}