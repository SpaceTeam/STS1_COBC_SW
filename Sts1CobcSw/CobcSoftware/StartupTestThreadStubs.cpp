#include <Sts1CobcSw/CobcSoftware/SpiStartupTestAndSupervisorThread.hpp>
#include <Sts1CobcSw/CobcSoftware/ThreadPriorities.hpp>
#include <Sts1CobcSw/FramSections/FramLayout.hpp>
#include <Sts1CobcSw/FramSections/PersistentVariables.hpp>
#include <Sts1CobcSw/Periphery/Fram.hpp>
#include <Sts1CobcSw/Utility/Debug.hpp>
#include <Sts1CobcSw/Utility/ErrorDetectionAndCorrection.hpp>
#include <Sts1CobcSw/Utility/RodosTime.hpp>

#include <rodos_no_using_namespace.h>


namespace sts1cobcsw
{
auto const stackSize = 800U;


class FramEpsStartupTestThread : public RODOS::StaticThread<stackSize>
{
public:
    FramEpsStartupTestThread()
        : StaticThread("FramEpsStartupTestThread", framEpsStartupTestThreadPriority)
    {
    }

private:
    void init() override
    {
    }

    void run() override
    {
        SuspendUntil(endOfTime);
        DEBUG_PRINT("FRAM/EPS start-up test ...");
        fram::framIsWorking.Store(true);
        persistentVariables.template Store<"epsIsWorking">(true);
        ResumeSpiStartupTestAndSupervisorThread();
        SuspendUntil(endOfTime);
    }
} framEpsStartupTestThread;


class FlashStartupTestThread : public RODOS::StaticThread<stackSize>
{
public:
    FlashStartupTestThread()
        : StaticThread("FlashStartupTestThread", flashStartupTestThreadPriority)
    {
    }

private:
    void init() override
    {
    }

    void run() override
    {
        SuspendUntil(endOfTime);
        DEBUG_PRINT("Flash start-up test ...");
        persistentVariables.template Store<"flashIsWorking">(true);
        ResumeSpiStartupTestAndSupervisorThread();
        SuspendUntil(endOfTime);
    }
} flashStartupTestThread;


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
        SuspendUntil(endOfTime);
        DEBUG_PRINT("RF start-up test ...");
        persistentVariables.template Store<"rfIsWorking">(true);
        ResumeSpiStartupTestAndSupervisorThread();
        SuspendUntil(endOfTime);
    }
} rfStartupTestThread;


auto ResumeFramEpsStartupTestThread() -> void
{
    framEpsStartupTestThread.resume();
}


auto ResumeFlashStartupTestThread() -> void
{
    flashStartupTestThread.resume();
}


auto ResumeRfStartupTestThread() -> void
{
    rfStartupTestThread.resume();
}
}
