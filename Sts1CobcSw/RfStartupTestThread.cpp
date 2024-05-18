#include <Sts1CobcSw/Periphery/Rf.hpp>
#include <Sts1CobcSw/ThreadPriorities.hpp>


namespace sts1cobcsw
{
constexpr auto stackSize = 100U;


class RfStartupTestThread : public RODOS::StaticThread<stackSize>
{
public:
    RfStartupTestThread() : StaticThread("RfStartupTestThread", rfStartupTestThreadPriority)
    {
    }

private:
    void init() override
    {
    }


    void run() override
    {
        // Suspend until EOT
        RODOS::AT(RODOS::END_OF_TIME);

        // Initialize device and read its ID
        rf::Initialize(TxType::packet);
        auto partNumber = rf::ReadPartNumber();
        if(partNumber != 0x4463)
        {
            rf::rfIsWorking = false;
        }

        // Wake up SPI startup test and supervisor thread

        // Suspend until EOT
        RODOS::AT(RODOS::END_OF_TIME);
    }
} RfStartupTestThread;
}
