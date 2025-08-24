#include <Sts1CobcSw/ErrorDetectionAndCorrection/EdacVariable.hpp>
#include <Sts1CobcSw/Firmware/StartupAndSpiSupervisorThread.hpp>
#include <Sts1CobcSw/Firmware/ThreadPriorities.hpp>
#include <Sts1CobcSw/Fram/Fram.hpp>
#include <Sts1CobcSw/FramSections/FramLayout.hpp>
#include <Sts1CobcSw/FramSections/PersistentVariables.hpp>
#include <Sts1CobcSw/RodosTime/RodosTime.hpp>
#include <Sts1CobcSw/Utility/DebugPrint.hpp>
#include <Sts1CobcSw/Vocabulary/Time.hpp>

#include <rodos_no_using_namespace.h>


namespace sts1cobcsw
{
namespace
{
auto const stackSize = 800U + EXTRA_SANITIZER_STACK_SIZE;


class FramEpsStartupTestThread : public RODOS::StaticThread<stackSize>
{
public:
    FramEpsStartupTestThread()
        : StaticThread("FramEpsStartupTestThread", framEpsStartupTestThreadPriority)
    {}


private:
    void init() override
    {}


    void run() override
    {
        SuspendUntil(endOfTime);
        DEBUG_PRINT("FRAM/EPS start-up test ...\n");
        fram::framIsWorking.Store(true);
        persistentVariables.Store<"epsIsWorking">(true);
        ResumeStartupAndSpiSupervisorThread();
        SuspendUntil(endOfTime);
    }
} framEpsStartupTestThread;


class FlashStartupTestThread : public RODOS::StaticThread<stackSize>
{
public:
    FlashStartupTestThread()
        : StaticThread("FlashStartupTestThread", flashStartupTestThreadPriority)
    {}


private:
    void init() override
    {}


    void run() override
    {
        SuspendUntil(endOfTime);
        DEBUG_PRINT("Flash start-up test ...\n");
        persistentVariables.Store<"flashIsWorking">(true);
        ResumeStartupAndSpiSupervisorThread();
        SuspendUntil(endOfTime);
    }
} flashStartupTestThread;


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
        persistentVariables.Store<"rfIsWorking">(true);
        ResumeStartupAndSpiSupervisorThread();
        SuspendUntil(endOfTime);
    }
} rfStartupTestThread;
}


auto ResumeFramEpsStartupTestThread() -> void  // NOLINT(*use-internal-linkage)
{
    framEpsStartupTestThread.resume();
}


auto ResumeFlashStartupTestThread() -> void  // NOLINT(*use-internal-linkage)
{
    flashStartupTestThread.resume();
}


auto ResumeRfStartupTestThread() -> void  // NOLINT(*use-internal-linkage)
{
    rfStartupTestThread.resume();
}
}
