#include <Sts1CobcSw/Dummy.hpp>

#include <rodos.h>


namespace sts1cobcsw
{
class HelloDummy : public StaticThread<>
{
    void run() override
    {
        auto const dummy = Dummy();

        TIME_LOOP(0, 500 * MILLISECONDS)
        {
            RODOS::PRINTF("Hello %s!\n", dummy.name.data());
        }
    }
};

auto const helloDummy = HelloDummy();
}
