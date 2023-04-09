#include <Sts1CobcSw/Periphery/Fram.hpp>

#include <Tests/HardwareTests/Utility.hpp>

#include <rodos_no_using_namespace.h>

#include <cstdint>


namespace sts1cobcsw
{
using RODOS::PRINTF;


decltype(periphery::fram::Initialize()) errorCode = 0;


class FramTest : public RODOS::StaticThread<>
{
public:
    FramTest() : StaticThread("FramTest")
    {
    }

private:
    void init() override
    {
        errorCode = periphery::fram::Initialize();
    }

    void run() override
    {
        PRINTF("\nFRAM test\n\n");

        PRINTF("Initialize(): %i == 0\n", static_cast<int>(errorCode));
        Check(errorCode == 0);
    }
} framTest;
}