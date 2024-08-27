#include <Tests/UnitTests/UnitTestThread.hpp>

#include <rodos_no_using_namespace.h>

#include <cstdint>
#include <cstdlib>


std::uint32_t printfMask = 0;


auto Require(bool condition, std::source_location location) -> void
{
    if(not condition)
    {
        RODOS::PRINTF("Test failed\n");
        RODOS::PRINTF("%s:%d: FAILED\n", location.file_name(), location.line());
        RODOS::isShuttingDown = true;
        std::exit(EXIT_FAILURE);  // NOLINT(concurrency-mt-unsafe)
    }
}


class UnitTestThread : public RODOS::StaticThread<>
{
public:
    UnitTestThread() : StaticThread("UnitTestThread")
    {
    }


private:
    auto init() -> void override
    {
    }

    auto run() -> void override
    {
        printfMask = 1;
        RunUnitTest();
        RODOS::PRINTF("Test passed\n");
        RODOS::isShuttingDown = true;
        std::exit(EXIT_SUCCESS);  // NOLINT(concurrency-mt-unsafe)
    }
} unitTestThread;
