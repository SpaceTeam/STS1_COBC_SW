#include <rodos_no_using_namespace.h>

#include <cstdint>


namespace sts1cobcsw
{
class HelloWorld : public RODOS::StaticThread<>
{
    void run() override
    {
        RODOS::PRINTF("Hello, World!\n");
        RODOS::hwResetAndReboot();
    }
} helloWorld;
}
