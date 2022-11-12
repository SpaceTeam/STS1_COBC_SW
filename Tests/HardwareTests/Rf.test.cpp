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
        periphery::rf::Initialize();
    }


    void run() override
    {
        PRINTF("\n");
        PRINTF("RF test\n");
        PRINTF("\n");

        if(periphery::rf::PartInfoIsCorrect())
        {
            PRINTF("Part info is correct.\n");
        }
        else
        {
            PRINTF("Part info is not correct.\n");
        }

        constexpr auto nMorses = 5;
        constexpr auto pauseDuration = 1 * RODOS::SECONDS;
        PRINTF("\n");
        PRINTF("Morsing %d times\n", nMorses);
        for(auto i = 0; i < nMorses; ++i)
        {
            periphery::rf::Morse();
            RODOS::AT(RODOS::NOW() + pauseDuration);
        }
    }
} rfTest;
}