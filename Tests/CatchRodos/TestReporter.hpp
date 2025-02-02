#pragma once


#include <Tests/CatchRodos/Assertion.hpp>
#include <Tests/CatchRodos/Stringification.hpp>
#include <Tests/CatchRodos/Vocabulary.hpp>

#include <rodos_no_using_namespace.h>

#include <etl/string.h>
#include <etl/vector.h>

#include <algorithm>
#include <numeric>


namespace sts1cobcsw
{
inline constexpr auto maxNTestCases = 100;


class TestReporter
{
public:
    static auto GetInstance() -> TestReporter &
    {
        static auto instance = TestReporter{};
        return instance;
    }


    static auto PrintPreamble() -> void
    {
#ifndef __linux__
        RODOS::PRINTF("\n\n");
#endif
    }


    static auto AddTestInit(gsl::czstring name) -> void
    {
        RODOS::PRINTF("TEST_INIT: %s\n", name);
    }


    auto AddTestCase(gsl::czstring name) -> void
    {
        RODOS::PRINTF("TEST_CASE: %s\n", name);
        testCases_.push_back(TestCase{});
    }


    auto AddPassedAssertion() -> void
    {
        testCases_.back().nPassedAssertions++;
    }


    template<typename LeftValue, typename RightValue>
    auto AddFailedAssertion(Assertion const & assertion,
                            LeftValue && leftValue,
                            RightValue && rightValue) -> void
    {
        testCases_.back().nFailedAssertions++;
        PrintFailMessage(
            assertion, std::forward<LeftValue>(leftValue), std::forward<RightValue>(rightValue));
    }


    auto NTotalTestCases() -> int
    {
        return static_cast<int>(testCases_.size());
    }


    auto NPassedTestCases() -> int
    {
        return static_cast<int>(std::count_if(testCases_.begin(),
                                              testCases_.end(),
                                              [](auto const & stats)
                                              { return stats.nFailedAssertions == 0; }));
    }


    auto NFailedTestCases() -> int
    {
        return static_cast<int>(std::count_if(testCases_.begin(),
                                              testCases_.end(),
                                              [](auto const & stats)
                                              { return stats.nFailedAssertions > 0; }));
    }


    auto NPassedAssertions() -> int
    {
        return std::accumulate(testCases_.begin(),
                               testCases_.end(),
                               0,
                               [](auto sum, auto const & stats)
                               { return sum + stats.nPassedAssertions; });
    }


    auto NFailedAssertions() -> int
    {
        return std::accumulate(testCases_.begin(),
                               testCases_.end(),
                               0,
                               [](auto sum, auto const & stats)
                               { return sum + stats.nFailedAssertions; });
    }


    auto NTotalAssertions() -> int
    {
        return NPassedAssertions() + NFailedAssertions();
    }


    auto PrintSummary() -> void
    {
        using RODOS::PRINTF;

        PRINTF(
            "================================================================================\n");
        if(NFailedTestCases() == 0)
        {
            PRINTF("All tests passed (%d assertion%s in %d test case%s)\n",
                   NTotalAssertions(),
                   NTotalAssertions() > 1 ? "s" : "",
                   NTotalTestCases(),
                   NTotalTestCases() > 1 ? "s" : "");
        }
        else
        {
            PRINTF("Test cases: %3d | %3d passed | %3d failed\n",
                   NTotalTestCases(),
                   NPassedTestCases(),
                   NFailedTestCases());
            PRINTF("Assertions: %3d | %3d passed | %3d failed\n",
                   NTotalAssertions(),
                   NPassedAssertions(),
                   NFailedAssertions());
        }
#ifndef __linux__
        PRINTF("\n");
#endif
    }


private:
    struct TestCase
    {
        int nPassedAssertions = 0;
        int nFailedAssertions = 0;
    };

    etl::vector<TestCase, maxNTestCases> testCases_;

    TestReporter() = default;


    template<typename LeftValue, typename RightValue>
    auto PrintFailMessage(Assertion const & assertion,
                          LeftValue && leftValue,
                          RightValue && rightValue)
    {
        using RODOS::PRINTF;

        if(testCases_.back().nFailedAssertions == 1)
        {
            PRINTF("\n");
        }
        PRINTF("%s:%d: FAILED:\n",
               assertion.location.file_name(),
               static_cast<int>(assertion.location.line()));
        PRINTF("  %s( %s )\n", assertion.macroName, assertion.expression);
        PRINTF("with expansion\n");
        auto leftValueString = ValueString("");
        // NOLINTNEXTLINE(*array-to-pointer-decay,*no-array-decay)
        Append(&leftValueString, std::forward<LeftValue>(leftValue));
        auto rightValueString = ValueString("");
        // NOLINTNEXTLINE(*array-to-pointer-decay,*no-array-decay)
        Append(&rightValueString, std::forward<RightValue>(rightValue));
        PRINTF("  %s %s %s\n\n", leftValueString.c_str(), assertion.op, rightValueString.c_str());
    }
};
}
