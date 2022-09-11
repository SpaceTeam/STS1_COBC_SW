#include "Lib.hpp"

#include <rodos.h>

uint32_t printfMask = 0;
namespace cobc
{
class HelloCobc : public StaticThread<>
{
    void run() override
    {
        printfMask = 1;
        auto const library = Library();

        PRINTF("Hello World from %s!\n", library.name.data());
        PRINTF("Hello World from %s!\n", library.shortName.c_str());
        hwResetAndReboot();
    }
};

auto const helloCobc = HelloCobc();
}
