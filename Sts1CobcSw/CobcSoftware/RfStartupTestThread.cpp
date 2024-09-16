#include <Sts1CobcSw/CobcSoftware/RfStartupTestThread.hpp>
#include <Sts1CobcSw/CobcSoftware/SpiStartupTestAndSupervisorThread.hpp>
#include <Sts1CobcSw/CobcSoftware/ThreadPriorities.hpp>
#include <Sts1CobcSw/FramSections/FramLayout.hpp>
#include <Sts1CobcSw/FramSections/PersistentVariables.hpp>
#include <Sts1CobcSw/Periphery/Rf.hpp>
#include <Sts1CobcSw/Utility/Debug.hpp>

#include <rodos_no_using_namespace.h>


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
        DEBUG_PRINT("RF start-up test ...");
        RODOS::AT(RODOS::END_OF_TIME);
        rf::Initialize(rf::TxType::packet);
        auto partNumber = rf::ReadPartNumber();
        if(partNumber != rf::correctPartNumber)
        {
            DEBUG_PRINT(" failed to read correct RF part number");
            persistentVariables.template Store<"rfIsWorking">(false);
        }
        ResumeSpiStartupTestAndSupervisorThread();
        RODOS::AT(RODOS::END_OF_TIME);
    }
} rfStartupTestThread;


auto ResumeRfStartupTestThread() -> void
{
    rfStartupTestThread.resume();
}
}
