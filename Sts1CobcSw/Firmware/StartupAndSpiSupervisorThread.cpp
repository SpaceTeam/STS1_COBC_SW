#include <Sts1CobcSw/Firmware/StartupAndSpiSupervisorThread.hpp>

#include <Sts1CobcSw/Edu/Edu.hpp>
#include <Sts1CobcSw/FileSystem/FileSystem.hpp>
#include <Sts1CobcSw/FileSystem/LfsMemoryDevice.hpp>
#include <Sts1CobcSw/Firmware/FlashStartupTestThread.hpp>
#include <Sts1CobcSw/Firmware/FramEpsStartupTestThread.hpp>
#include <Sts1CobcSw/Firmware/RfStartupTestThread.hpp>
#include <Sts1CobcSw/Firmware/ThreadPriorities.hpp>
#include <Sts1CobcSw/Fram/Fram.hpp>
#include <Sts1CobcSw/FramSections/FramLayout.hpp>
#include <Sts1CobcSw/FramSections/PersistentVariables.hpp>
#include <Sts1CobcSw/Hal/Spi.hpp>
#include <Sts1CobcSw/Hal/Spis.hpp>
#include <Sts1CobcSw/Outcome/Outcome.hpp>
#include <Sts1CobcSw/RodosTime/RodosTime.hpp>
#include <Sts1CobcSw/Utility/DebugPrint.hpp>
#include <Sts1CobcSw/Utility/ErrorDetectionAndCorrection.hpp>
#include <Sts1CobcSw/Vocabulary/Time.hpp>
#ifndef __linux__
    #include <Sts1CobcSw/WatchdogTimers/WatchdogTimers.hpp>
#endif

#include <strong_type/affine_point.hpp>
#include <strong_type/difference.hpp>
#include <strong_type/ordered.hpp>
#include <strong_type/type.hpp>

#include <rodos_no_using_namespace.h>

#include <compare>


namespace sts1cobcsw
{
namespace
{
// Running the SpiSupervisor HW test in debug mode showed a max. stack usage of < 1600 B. The golden
// test needs < 2400 B.
constexpr auto stackSize = 2500 + EXTRA_SANITIZER_STACK_SIZE;

inline constexpr auto initialSleepTime = 10 * ms;
// TODO: Think about how often the supervision should run
constexpr auto supervisionPeriod = 1 * s;


auto ExecuteStartupTests() -> void;
auto ExecuteStartupTest(void (*startupTestThreadResumeFuntion)()) -> bool;
auto InitializeAndFeedResetDog() -> void;
auto SetUpFileSystem() -> void;


class StartupAndSpiSupervisorThread : public RODOS::StaticThread<stackSize>
{
public:
    StartupAndSpiSupervisorThread()
        : StaticThread("StartupAndSpiSupervisorThread", startupAndSpiSupervisorThreadPriority)
    {}


private:
    void run() override
    {
        // Briefly go to sleep to ensure that the low-priority startup test threads have started and
        // are waiting for the high-priority supervisor thread to resume them
        SuspendFor(initialSleepTime);
        ExecuteStartupTests();
        DEBUG_PRINT_STACK_USAGE();
        if(not persistentVariables.Load<"rfIsWorking">())
        {
            DEBUG_PRINT("Resetting and rebooting in 2 s\n");
            // TODO: Add a named constant for this delay
            SuspendFor(2 * s);
            RODOS::hwResetAndReboot();
        }
        InitializeAndFeedResetDog();
        SetUpFileSystem();
        edu::Initialize();
        auto i = 0U;
        TIME_LOOP(0, value_of(supervisionPeriod))
        {
            auto timeoutHappened = false;
            auto now = CurrentRodosTime();
            if(now > framEpsSpi.TransferEnd())
            {
                DEBUG_PRINT("FRAM/EPS SPI timeout occurred\n");
                timeoutHappened = true;
            }
            if(now > flashSpi.TransferEnd())
            {
                DEBUG_PRINT("Flash SPI timeout occurred\n");
                timeoutHappened = true;
                persistentVariables.Increment<"nFlashErrors">();
            }
            if(now > rfSpi.TransferEnd())
            {
                DEBUG_PRINT("RF SPI timeout occurred\n");
                timeoutHappened = true;
                persistentVariables.Increment<"nRfErrors">();
            }
            if(timeoutHappened)
            {
                RODOS::hwResetAndReboot();
            }
            if(i < 2)
            {
                DEBUG_PRINT_STACK_USAGE();
                ++i;
            }
        }
    }
} startupAndSpiSupervisorThread;
}


auto ResumeStartupAndSpiSupervisorThread() -> void
{
    startupAndSpiSupervisorThread.resume();
}


namespace
{
auto ExecuteStartupTests() -> void
{
    // The messages are only used in DEBUG_PRINT() calls, so they are unused in release builds
    [[maybe_unused]] static constexpr auto errorMessage = " failed to complete in time\n";
    [[maybe_unused]] static constexpr auto successMessage = " completed in time\n";

    auto testWasSuccessful = ExecuteStartupTest(ResumeFramEpsStartupTestThread);
    DEBUG_PRINT(fram::framIsWorking.Load() ? " " : " and");
    if(not testWasSuccessful)
    {
        DEBUG_PRINT("%s", errorMessage);
        fram::framIsWorking.Store(false);
        persistentVariables.Store<"epsIsWorking">(false);
    }
    else
    {
        DEBUG_PRINT("%s", successMessage);
    }

    testWasSuccessful = ExecuteStartupTest(ResumeFlashStartupTestThread);
    DEBUG_PRINT(persistentVariables.Load<"flashIsWorking">() ? " " : " and");
    if(not testWasSuccessful)
    {
        DEBUG_PRINT("%s", errorMessage);
        persistentVariables.Store<"flashIsWorking">(false);
        persistentVariables.Increment<"nFlashErrors">();
    }
    else
    {
        DEBUG_PRINT("%s", successMessage);
    }

    testWasSuccessful = ExecuteStartupTest(ResumeRfStartupTestThread);
    DEBUG_PRINT(persistentVariables.Load<"rfIsWorking">() ? " " : " and");
    if(not testWasSuccessful)
    {
        DEBUG_PRINT("%s", errorMessage);
        persistentVariables.Store<"rfIsWorking">(false);
        persistentVariables.Increment<"nRfErrors">();
    }
    else
    {
        DEBUG_PRINT("%s", successMessage);
    }
}


auto ExecuteStartupTest(void (*startupTestThreadResumeFuntion)()) -> bool
{
    auto testEnd = CurrentRodosTime() + startupTestTimeout;
    startupTestThreadResumeFuntion();
    SuspendUntil(testEnd);
    return CurrentRodosTime() <= testEnd;
}


auto InitializeAndFeedResetDog() -> void
{
#ifndef __linux__
    rdt::Initialize();
    // We need to feed the reset dog right away since the startup tests can take up to 650 ms
    rdt::Feed();
#endif
}


auto SetUpFileSystem() -> void
{
    fs::Initialize();
    auto mountResult = fs::Mount();
    if(mountResult.has_error())
    {
        DEBUG_PRINT("Failed to mount file system: %s\n", ToCZString(mountResult.error()));
        persistentVariables.Increment<"nFileSystemErrors">();
    }
    auto createDirectoryResult = fs::CreateDirectory("/programs");
    if(createDirectoryResult.has_error()
       and createDirectoryResult.error() != ErrorCode::alreadyExists)
    {
        DEBUG_PRINT("Failed to create directory /programs: %s\n",
                    ToCZString(createDirectoryResult.error()));
        persistentVariables.Increment<"nFileSystemErrors">();
    }
    createDirectoryResult = fs::CreateDirectory("/results");
    if(createDirectoryResult.has_error()
       and createDirectoryResult.error() != ErrorCode::alreadyExists)
    {
        DEBUG_PRINT("Failed to create directory /results: %s\n",
                    ToCZString(createDirectoryResult.error()));
        persistentVariables.Increment<"nFileSystemErrors">();
    }
}
}
}
