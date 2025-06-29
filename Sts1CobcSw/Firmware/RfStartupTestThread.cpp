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
// Running the SpiSupervisor HW test in debug mode showed that the minimum required stack size is
// between 1400 and 1800 bytes
constexpr auto stackSize = 1800;


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
        auto testSucceeded = [&]() -> bool
        {
            auto initializeResult = rf::Initialize(rf::TxType::packet);
            if(initializeResult.has_error())
            {
                DEBUG_PRINT(" failed to initialize RF module");
                return false;
            }
            auto partNumber = rf::ReadPartNumber();
            if(partNumber != rf::correctPartNumber)
            {
                DEBUG_PRINT(" failed to read correct RF part number");
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
