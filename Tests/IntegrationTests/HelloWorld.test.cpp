#include <rodos_no_using_namespace.h>


namespace sts1cobcsw
{
namespace
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
}
