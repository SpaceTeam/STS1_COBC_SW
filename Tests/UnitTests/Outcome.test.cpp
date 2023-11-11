// Test the outcome library

#include <catch2/catch_test_macros.hpp>
#include <outcome-experimental.hpp>

#include <iostream>

// First define a policy
struct AbortPolicy : outcome_v2::experimental::policy::base
{
    template<class Impl>
    // NOLINTNEXTLINE(readability-identifier-naming)
    static constexpr void wide_value_check(Impl && self)
    {
        //! Call RODOS::hwResetAndReboot() whenever .value() is called on an object that does not
        //! contain a value
        if(!base::_has_value(std::forward<Impl>(self)))
        {
            std::abort();
        }
    }

    template<class Impl>
    // NOLINTNEXTLINE(readability-identifier-naming)
    static constexpr void wide_error_check(Impl && self)
    {
        //! Call RODOS::hwResetAndReboot() whenever .error() is called on an object that does not
        //! contain an error
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


auto Convert(const std::string & str) noexcept -> Result<int>
{
    if(str.empty())
    {
        return ConversionErrc::emptyString;
    }

    if(!std::all_of(str.begin(), str.end(), ::isdigit))
    {
        return ConversionErrc::illegalChar;
    }

    if(str.length() > 9)
    {
        return ConversionErrc::tooLong;
    }

    // NOLINTNEXTLINE (cert-err34-c)
    return atoi(str.c_str());
}


TEST_CASE("Inspecting result")
{
    Result<int> result = outcome_v2::success();
    REQUIRE(result.has_value());
    REQUIRE(result);  // Boolean cast

    // Test each defined conversion error
    result = Convert("abc");
    REQUIRE(result.has_error());
    REQUIRE(result.error() == ConversionErrc::illegalChar);

    result = Convert("314159265359");
    REQUIRE(result.has_error());
    REQUIRE(result.error() == ConversionErrc::tooLong);

    result = Convert("");
    REQUIRE(result.has_error());
    REQUIRE(result.error() == ConversionErrc::emptyString);


    // Test success
    result = Convert("278");
    REQUIRE(result.has_value());
    REQUIRE(result);  // Boolean cast
    REQUIRE(result.value() == 278);
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


// Dummy function to chain with Convert
// Return type is a pair just to display how OUTCOME_TRY works with different return<> types
auto Add(int op1, const std::string & str) -> Result<std::pair<int, int>>
{
    // From https://ned14.github.io/outcome/tutorial/essential/result/inspecting/
    // Our control statement means:
    // if Convert() returned failure, this same error information should be returned from Add(),
    // even though Add() and Convert() have different result<> types.
    // If Convert returned success, we create variable op2 of type int with the value returned from
    // Convert. If control goes to subsequent line, it means Convert succeeded and variable of type
    // BigInt is in scope.
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
    auto result1 = Write(/*shouldSucceed=*/false);
    assert(result1.has_error());
    REQUIRE(result1.error() == ConversionErrc::illegalChar);

    // Test success
    auto result2 = Write(/*shouldSucceed=*/true);
    assert(result2.has_value());
    assert(result2);
    REQUIRE(result2.value() == 1);

    // Test failure
    auto result3 = Add(1, "3.14");
    REQUIRE(not result3.has_value());
    REQUIRE(result3.has_error());
    REQUIRE(result3.error() == ConversionErrc::illegalChar);

    // Test success
    auto result4 = Add(1, "278");
    REQUIRE(result4.has_value());
    REQUIRE(result4);
    REQUIRE(result4.value().first == 279);
}
