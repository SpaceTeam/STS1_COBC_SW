#include <Sts1CobcSw/Periphery/Edu.hpp>

#include <rodos_no_using_namespace.h>


namespace sts1cobcsw
{
class UpdateTimeTest : public RODOS::StaticThread<>
{
    void run() override
    {
        auto eduUart = periphery::Edu();

        RODOS::PRINTF("START\n");
        constexpr auto timestamp = 0x12345678;
        eduUart.UpdateTime(timestamp);
        RODOS::PRINTF("\nDONE\n");
    }
};


auto const updateTimeTest = UpdateTimeTest();
}