#include <rodos.h>


uint32_t printfMask = 0;

namespace sts1cobcsw
{
class HelloWorld : public StaticThread<>
{
    void run() override
    {
        printfMask = 1;
        RODOS::PRINTF("Hello, World!\n");
        hwResetAndReboot();
    }
};

auto const helloWorld = HelloWorld();
}
