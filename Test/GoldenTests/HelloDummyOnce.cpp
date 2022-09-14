#include <Sts1CobcSw/Dummy.hpp>

#include <rodos.h>


uint32_t printfMask = 0;

namespace sts1cobcsw
{
class HelloDummyOnce : public StaticThread<>
{
    void run() override
    {
        printfMask = 1;
        auto const dummy = Dummy();

        PRINTF("Hello, %s!\n", dummy.name.data());
        hwResetAndReboot();
    }
};

auto const helloDummyOnce = HelloDummyOnce();
}
