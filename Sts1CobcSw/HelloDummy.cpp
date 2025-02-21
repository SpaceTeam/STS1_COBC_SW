#include <Sts1CobcSw/Dummy.hpp>
#include <Sts1CobcSw/Utility/DebugPrint.hpp>

#include <rodos_no_using_namespace.h>

#include <etl/string.h>

#include <cinttypes>  // IWYU pragma: keep


namespace RODOS
{
#ifdef __linux__
HAL_UART uart_stdout(RODOS::UART_IDX2);  // NOLINT
#else
extern HAL_UART uart_stdout;  // NOLINT
#endif
}


namespace sts1cobcsw
{
class HelloDummy : public RODOS::StaticThread<>
{
    void run() override
    {
        auto const dummy = Dummy();

        TIME_LOOP(0, 500 * RODOS::MILLISECONDS)
        {
            RODOS::PRINTF("Hello %s!\n", dummy.name.data());
            DEBUG_PRINT("Debug printing ! printfMask = %" PRIu32 "\n", printfMask);
        }
    }
} helloDummy;
}
