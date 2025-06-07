#include <Tests/CatchRodos/TestRegistration.hpp>
#include <Tests/CatchRodos/TestReporter.hpp>

#include <rodos_no_using_namespace.h>

#include <cstdint>
#include <cstdlib>


std::uint32_t printfMask = 0;


namespace sts1cobcsw
{
namespace
{
// We really don't want stack overflows in tests, so we are generous with the stack size and use
// about half of the available RAM (128 kiB) on the STM32F411RE
constexpr auto stackSize = 64'000U;


class TestThread : public RODOS::StaticThread<stackSize>
{
public:
    TestThread() : StaticThread("TestThread")
    {}


private:
    auto init() -> void override
    {
        printfMask = 1;
        TestReporter::GetInstance().PrintPreamble();
        TestRegistry::GetInstance().RunTestInits();
        printfMask = 0;
    }


    auto run() -> void override
    {
        printfMask = 1;
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
}
