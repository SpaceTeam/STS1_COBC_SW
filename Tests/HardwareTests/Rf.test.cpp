#include <Sts1CobcSw/Periphery/Rf.hpp>

#include <rodos_no_using_namespace.h>


namespace sts1cobcsw
{
using RODOS::PRINTF;


class RfTest : public RODOS::StaticThread<>
{
public:
    RfTest() : StaticThread("RfTest")
    {
    }

private:
    void init() override
    {
        RODOS::PRINTF("Hello RfTest\n");
    }


    void run() override
    {
        periphery::rf::Initialize();
        RODOS::PRINTF("Si4463 initialized\n");

        constexpr auto nMorses = 5;
        constexpr auto pauseDuration = 1 * RODOS::SECONDS;
        PRINTF("Morsing %d times\n", nMorses);
        for(auto i = 0; i < nMorses; ++i)
        {
            RODOS::PRINTF("Morsing...\n");
            periphery::rf::Morse();
            RODOS::AT(RODOS::NOW() + pauseDuration);
        }
        RODOS::PRINTF("Morsing done\n");
    }
} rfTest;
}