#include <Sts1CobcSw/Dummy.hpp>

#include <rodos_no_using_namespace.h>

#include <span>
#include <string_view>

namespace RODOS
{
#if defined(LINUX_SYSTEM)
HAL_UART uart_stdout(RODOS::UART_IDX2);  // NOLINT
#elif defined(GENERIC_SYSTEM)
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
        }
    }
};

auto const helloDummy = HelloDummy();
}
