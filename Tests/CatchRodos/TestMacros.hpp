#pragma once


#include <Tests/CatchRodos/AssertionDecomposer.hpp>
#include <Tests/CatchRodos/TestRegistration.hpp>


// NOLINTBEGIN(cppcoreguidelines-macro-usage)
#define INTERNAL_CATCH_RODOS_UNIQUE_NAME(prefix) INTERNAL_CATCH_RODOS_COMBINE(prefix, __LINE__)
#define INTERNAL_CATCH_RODOS_COMBINE(left, right) INTERNAL_CATCH_RODOS_COMBINE_HELPER(left, right)
#define INTERNAL_CATCH_RODOS_COMBINE_HELPER(left, right) left##right

#define TEST_INIT(name) \
    static void INTERNAL_CATCH_RODOS_UNIQUE_NAME(INTERNAL_CATCH_RODOS_TEST_INIT_)(); \
    static sts1cobcsw::TestInitRegistrar INTERNAL_CATCH_RODOS_UNIQUE_NAME( \
        INTERNAL_CATCH_RODOS_TEST_INIT_REGISTRAR_)( \
        INTERNAL_CATCH_RODOS_UNIQUE_NAME(INTERNAL_CATCH_RODOS_TEST_INIT_), name); \
    static void INTERNAL_CATCH_RODOS_UNIQUE_NAME(INTERNAL_CATCH_RODOS_TEST_INIT_)()

#define TEST_CASE(name) \
    static void INTERNAL_CATCH_RODOS_UNIQUE_NAME(INTERNAL_CATCH_RODOS_TEST_CASE_)(); \
    static sts1cobcsw::TestCaseRegistrar INTERNAL_CATCH_RODOS_UNIQUE_NAME( \
        INTERNAL_CATCH_RODOS_TEST_CASE_REGISTRAR_)( \
        INTERNAL_CATCH_RODOS_UNIQUE_NAME(INTERNAL_CATCH_RODOS_TEST_CASE_), name); \
    static void INTERNAL_CATCH_RODOS_UNIQUE_NAME(INTERNAL_CATCH_RODOS_TEST_CASE_)()

// NOLINTBEGIN(bugprone-macro-parentheses)
#define CHECK(expr) \
    do \
    { \
        _Pragma("GCC diagnostic push"); \
        _Pragma("GCC diagnostic ignored \"-Wparentheses\""); \
        _Pragma("GCC diagnostic ignored \"-Wunused\""); \
        _Pragma("GCC diagnostic ignored \"-Wunused-value\""); \
        (sts1cobcsw::AssertionDecomposer{"CHECK", #expr} < expr) == true; \
        _Pragma("GCC diagnostic pop"); \
    } while(false)

#define REQUIRE(expr) \
    do \
    { \
        _Pragma("GCC diagnostic push"); \
        _Pragma("GCC diagnostic ignored \"-Wparentheses\""); \
        _Pragma("GCC diagnostic ignored \"-Wunused\""); \
        if(not((sts1cobcsw::AssertionDecomposer{"REQUIRE", #expr} < expr) == true)) \
            return; \
        _Pragma("GCC diagnostic pop"); \
    } while(false)
// NOLINTEND(bugprone-macro-parentheses)
// NOLINTEND(cppcoreguidelines-macro-usage)
