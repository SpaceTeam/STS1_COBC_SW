#include <Sts1CobcSw/Dummy.hpp>

#include <rodos_no_using_namespace.h>

#include <etl/string.h>


namespace sts1cobcsw
{
class HelloDummy : public RODOS::StaticThread<>
{
    void run() override
    {
        auto const dummy = Dummy();

        RODOS::PRINTF("Hello, %s!\n", dummy.name.data());
        RODOS::hwResetAndReboot();
    }
} helloDummy;
}
