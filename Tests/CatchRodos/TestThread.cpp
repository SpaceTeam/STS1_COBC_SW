#include <Tests/CatchRodos/RfLatchupDisablePin.hpp>
#include <Tests/CatchRodos/TestRegistration.hpp>
#include <Tests/CatchRodos/TestReporter.hpp>

#include <rodos_no_using_namespace.h>

#include <cstdint>
#include <cstdlib>


std::uint32_t printfMask = 0;


namespace sts1cobcsw
{
class TestThread : public RODOS::StaticThread<>
{
public:
    TestThread() : StaticThread("TestThread")
    {
    }


private:
    auto init() -> void override
    {
        printfMask = 1;
        InitializeRfLatchupDisablePins();
        TestReporter::GetInstance().PrintPreamble();
        TestRegistry::GetInstance().RunTestInits();
        printfMask = 0;
    }


    auto run() -> void override
    {
        printfMask = 1;
        EnableRfLatchupProtection();
        TestRegistry::GetInstance().RunTestCases();
        TestReporter::GetInstance().PrintSummary();
#ifdef __linux__
        RODOS::isShuttingDown = true;
        // NOLINTNEXTLINE(concurrency-mt-unsafe)
        std::exit(TestReporter::GetInstance().NFailedTestCases());
#endif
    }
} testThread;
}
