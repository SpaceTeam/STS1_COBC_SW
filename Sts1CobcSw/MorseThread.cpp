#include <Sts1CobcSw/Periphery/Rf.hpp>
#include <Sts1CobcSw/Periphery/RfNames.hpp>

#include <rodos_no_using_namespace.h>


namespace sts1cobcsw
{
class MorseThread : public RODOS::StaticThread<>
{
public:
    MorseThread() : StaticThread("MorseThread")
    {
    }

private:
    void init() override
    {
    }


    void run() override
    {
        RODOS::PRINTF("Initializing\n");
        periphery::rf::Initialize(periphery::rf::TxType::morse);

        while(true)
        {
            RODOS::PRINTF("Morsing ...\n");
            periphery::rf::Morse(periphery::rf::callSign);
            periphery::rf::Morse(" portable Test");
            RODOS::AT(RODOS::NOW() + 2 * RODOS::SECONDS);
        }
    }
} morseThread;
}
