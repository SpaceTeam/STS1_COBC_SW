#include <rodos_no_using_namespace.h>

#include <cstdint>


std::uint32_t printfMask = 0;


namespace sts1cobcsw
{
class HelloWorld : public RODOS::StaticThread<>
{
    void run() override
    {
        printfMask = 1;
        RODOS::PRINTF("Hello, World!\n");
        RODOS::hwResetAndReboot();
    }
};

auto const helloWorld = HelloWorld();
}
