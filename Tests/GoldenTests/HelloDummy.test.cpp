#include <Sts1CobcSw/Dummy.hpp>

#include <rodos_no_using_namespace.h>

#include <etl/string.h>

#include <cstdint>


std::uint32_t printfMask = 0;


namespace sts1cobcsw
{
class HelloDummy : public RODOS::StaticThread<>
{
    void run() override
    {
        printfMask = 1;
        auto const dummy = Dummy();

        RODOS::PRINTF("Hello, %s!\n", dummy.name.data());
        RODOS::hwResetAndReboot();
    }
} helloDummy;
}
