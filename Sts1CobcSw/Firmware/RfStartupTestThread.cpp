#include <Sts1CobcSw/Firmware/RfStartupTestThread.hpp>

#include <Sts1CobcSw/Firmware/SpiStartupTestAndSupervisorThread.hpp>
#include <Sts1CobcSw/Firmware/ThreadPriorities.hpp>
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
// Running the SpiSupervisor HW test in debug mode showed a max. stack usage of < 1500 B
constexpr auto stackSize = 2000;


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
        DEBUG_PRINT("RF start-up test ...\n");
        auto testSucceeded = [&]() -> bool
        {
            auto initializeResult = rf::Initialize(rf::TxType::packet);
            if(initializeResult.has_error())
            {
                DEBUG_PRINT("  failed to initialize RF module\n");
                return false;
            }
            auto partNumber = rf::ReadPartNumber();
            if(partNumber != rf::correctPartNumber)
            {
                DEBUG_PRINT("  failed to read correct RF part number\n");
                return false;
            }
            return true;
        }();
        if(testSucceeded)
        {
            persistentVariables.Store<"rfIsWorking">(true);
        }
        else
        {
            persistentVariables.Store<"rfIsWorking">(false);
            persistentVariables.Increment<"nRfErrors">();
        }
        DEBUG_PRINT_STACK_USAGE();
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
