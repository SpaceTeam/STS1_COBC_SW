// Test the outcome library

#include <catch2/catch_test_macros.hpp>
#include <outcome-experimental.hpp>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <limits>
#include <string>
#include <utility>


// First define a policy
struct AbortPolicy : outcome_v2::experimental::policy::base
{
    template<class Impl>
    // NOLINTNEXTLINE(readability-identifier-naming)
    static constexpr void wide_value_check(Impl && self)
    {
        if(!base::_has_value(std::forward<Impl>(self)))
        {
            std::abort();
        }
    }

    template<class Impl>
    // NOLINTNEXTLINE(readability-identifier-naming)
    static constexpr void wide_error_check(Impl && self)
    {
        if(!base::_has_error(std::forward<Impl>(self)))
        {
            std::abort();
        }
    }

    template<class Impl>
    // NOLINTNEXTLINE(readability-identifier-naming)
    static constexpr void wide_exception_check(Impl && self)
    {
        if(!base::_has_exception(std::forward<Impl>(self)))
        {
            std::abort();
        }
    }
};


enum class ConversionErrc
{
    success = 0,      // 0 should not represent an error
    emptyString = 1,  // (for rationale, see tutorial on error codes)
    illegalChar = 2,
    tooLong = 3,
};


template<typename T>
using Result = outcome_v2::experimental::status_result<T, ConversionErrc, AbortPolicy>;


auto Convert(std::string const & str) noexcept -> Result<int>
{
    if(str.empty())
    {
        return ConversionErrc::emptyString;
    }

    // NOLINTNEXTLINE(readability-identifier-length)
    if(!std::all_of(str.begin(), str.end(), [](unsigned char c) { return std::isdigit(c); }))
    {
        return ConversionErrc::illegalChar;
    }

    if(str.length() > std::numeric_limits<int>::digits10)
    {
        return ConversionErrc::tooLong;
    }

    // NOLINTNEXTLINE(cert-err34-c)
    return atoi(str.c_str());
}


// NOLINTNEXTLINE(cert-err58-cpp)
TEST_CASE("Inspecting result")
{
    Result<int> result = outcome_v2::success();
    CHECK(result.has_value());
    CHECK(result);  // Boolean cast

    // Test each defined conversion error
    result = Convert("abc");
    CHECK(result.has_error());
    CHECK(result.error() == ConversionErrc::illegalChar);

    result = Convert("314159265359");
    CHECK(result.has_error());
    CHECK(result.error() == ConversionErrc::tooLong);

    result = Convert("");
    CHECK(result.has_error());
    CHECK(result.error() == ConversionErrc::emptyString);


    // Test success
    result = Convert("278");
    CHECK(result.has_value());
    CHECK(result);  // Boolean cast
    CHECK(result.value() == 278);
}


// Dummy function to chain with Convert
auto WriteData(int * buffer, bool shouldSucceed) -> Result<void>
{
    if(shouldSucceed)
    {
        *buffer = 1;
        return outcome_v2::success();
    }
    // Return some dummy error here, just to check that it is handled correctly
    return ConversionErrc::emptyString;
}


// Dummy function to chain with Convert. Return type is a pair just to display how OUTCOME_TRY works
// with different return types
auto Add(int op1, std::string const & str) -> Result<std::pair<int, int>>
{
    // From https://ned14.github.io/outcome/tutorial/essential/result/inspecting/
    // Our control statement means: if Convert() returned failure, this same error information
    // should be returned from Add(), even though Add() and Convert() have different result<> types.
    // If Convert() returned success, we create variable op2 of type int with the value returned
    // from Convert(). If control goes to subsequent line, it means Convert() succeeded and variable
    // of type int is in scope.
    OUTCOME_TRY(auto op2, Convert(str));
    return std::make_pair(op1 + op2, op2);
}


auto Write(bool shouldSucceed) -> Result<int>
{
    // Same thing as in Add(), but this time the function that we are calling returns void in case
    // of success, thus we do not need to provide two parameters to OUTCOME_TRY
    int buffer = 0;
    OUTCOME_TRY(WriteData(&buffer, shouldSucceed));
    return buffer;
}


TEST_CASE("TRY macro")
{
    // Test failure
    auto result1 = Write(false);
    CHECK(result1.has_error());
    CHECK(result1.error() == ConversionErrc::emptyString);

    // Test success
    auto result2 = Write(true);
    CHECK(result2.has_value());
    CHECK(result2);
    CHECK(result2.value() == 1);

    // Test failure
    auto result3 = Add(1, "3.14");
    CHECK(not result3.has_value());
    CHECK(result3.has_error());
    CHECK(result3.error() == ConversionErrc::illegalChar);

    // Test success
    auto result4 = Add(1, "278");
    CHECK(result4.has_value());
    CHECK(result4);
    CHECK(result4.value().first == 279);
}
