#include <Sts1CobcSw/Dummy.hpp>

#include <Sts1CobcSw/Util/Util.hpp>

#include <rodos.h>

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
class HelloDummy : public StaticThread<>
{
    void run() override
    {
        auto const dummy = Dummy();

        using std::operator""sv;

        TIME_LOOP(0, 1000 * MILLISECONDS)
        {
            // RODOS::PRINTF("Hello %s!\n", dummy.name.data());
            util::WriteTo(&RODOS::uart_stdout, "Hello from uart2\n");
        }
    }
};

auto const helloDummy = HelloDummy();
}
