#include <Sts1CobcSw/CobcSoftware/RfStartupTestThread.hpp>

#include <Sts1CobcSw/CobcSoftware/SpiStartupTestAndSupervisorThread.hpp>
#include <Sts1CobcSw/CobcSoftware/ThreadPriorities.hpp>
#include <Sts1CobcSw/FramSections/FramLayout.hpp>
#include <Sts1CobcSw/FramSections/PersistentVariables.hpp>
#include <Sts1CobcSw/Rf/Rf.hpp>
#include <Sts1CobcSw/RodosTime/RodosTime.hpp>
#include <Sts1CobcSw/Utility/DebugPrint.hpp>
#include <Sts1CobcSw/Vocabulary/Time.hpp>

#include <rodos_no_using_namespace.h>


namespace sts1cobcsw
{
namespace
{
// Running the SpiSupervisor HW test in debug mode showed that the minimum required stack size is
// between 900 and 1000 bytes
constexpr auto stackSize = 1000;


class RfStartupTestThread : public RODOS::StaticThread<stackSize>
{
public:
    RfStartupTestThread() : StaticThread("RfStartupTestThread", rfStartupTestThreadPriority)
    {}


private:
    void init() override
    {}


    void run() override
    {
        SuspendUntil(endOfTime);
        DEBUG_PRINT("RF start-up test ...");
        rf::Initialize(rf::TxType::packet);
        auto partNumber = rf::ReadPartNumber();
        if(partNumber == rf::correctPartNumber)
        {
            persistentVariables.template Store<"rfIsWorking">(true);
        }
        else
        {
            DEBUG_PRINT(" failed to read correct RF part number");
            persistentVariables.template Store<"rfIsWorking">(false);
        }
        ResumeSpiStartupTestAndSupervisorThread();
        SuspendUntil(endOfTime);
    }
} rfStartupTestThread;
}


auto ResumeRfStartupTestThread() -> void
{
    rfStartupTestThread.resume();
}
}
