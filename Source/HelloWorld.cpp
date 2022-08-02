#include "Lib.hpp"

#include <rodos.h>


namespace cobc
{
class HelloWorld : public StaticThread<>
{
    void run() override
    {
        auto const library = Library();
        auto toggle = true;

        TIME_LOOP(0, 500 * MILLISECONDS)
        {
            if(toggle)
            {
                PRINTF("Hello World from %s!\n", library.name.data());
            }
            else
            {
                PRINTF("Hello World from %s!\n", library.shortName.c_str());
            }
            toggle = not toggle;
        }
    }
};

auto const helloWorld = HelloWorld();
}
