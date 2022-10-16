#include <Sts1CobcSw/Dummy.hpp>

#include <rodos_no_using_namespace.h>


namespace sts1cobcsw
{
class HelloDummy : public RODOS::StaticThread<>
{
    void run() override
    {
        auto const dummy = Dummy();

        TIME_LOOP(0, 500 * RODOS::MILLISECONDS)
        {
            RODOS::PRINTF("Hello %s!\n", dummy.name.data());
        }
    }
};

auto const helloDummy = HelloDummy();
}
