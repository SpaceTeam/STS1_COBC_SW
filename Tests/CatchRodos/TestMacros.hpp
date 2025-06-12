#pragma once


#include <Tests/CatchRodos/AssertionDecomposer.hpp>
#include <Tests/CatchRodos/TestRegistration.hpp>  // IWYU pragma: export


// NOLINTBEGIN(cppcoreguidelines-macro-usage)
#define INTERNAL_CATCH_RODOS_UNIQUE_NAME(prefix) INTERNAL_CATCH_RODOS_COMBINE(prefix, __LINE__)
#define INTERNAL_CATCH_RODOS_COMBINE(left, right) INTERNAL_CATCH_RODOS_COMBINE_HELPER(left, right)
#define INTERNAL_CATCH_RODOS_COMBINE_HELPER(left, right) left##right

#define TEST_INIT(name) \
    /* NOLINTNEXTLINE(misc-use-anonymous-namespace) */ \
    static void INTERNAL_CATCH_RODOS_UNIQUE_NAME(INTERNAL_CATCH_RODOS_TEST_INIT_)(); \
    /* NOLINTNEXTLINE(misc-use-anonymous-namespace) */ \
    static sts1cobcsw::TestInitRegistrar INTERNAL_CATCH_RODOS_UNIQUE_NAME( \
        INTERNAL_CATCH_RODOS_TEST_INIT_REGISTRAR_)( \
        INTERNAL_CATCH_RODOS_UNIQUE_NAME(INTERNAL_CATCH_RODOS_TEST_INIT_), name); \
    /* NOLINTNEXTLINE(misc-use-anonymous-namespace) */ \
    static void INTERNAL_CATCH_RODOS_UNIQUE_NAME(INTERNAL_CATCH_RODOS_TEST_INIT_)()


#define TEST_CASE(name) \
    /* NOLINTNEXTLINE(misc-use-anonymous-namespace) */ \
    static void INTERNAL_CATCH_RODOS_UNIQUE_NAME(INTERNAL_CATCH_RODOS_TEST_CASE_)(); \
    /* NOLINTNEXTLINE(misc-use-anonymous-namespace) */ \
    static sts1cobcsw::TestCaseRegistrar INTERNAL_CATCH_RODOS_UNIQUE_NAME( \
        INTERNAL_CATCH_RODOS_TEST_CASE_REGISTRAR_)( \
        INTERNAL_CATCH_RODOS_UNIQUE_NAME(INTERNAL_CATCH_RODOS_TEST_CASE_), name); \
    /* NOLINTNEXTLINE(misc-use-anonymous-namespace) */ \
    static void INTERNAL_CATCH_RODOS_UNIQUE_NAME(INTERNAL_CATCH_RODOS_TEST_CASE_)()

// NOLINTBEGIN(bugprone-macro-parentheses)
#define CHECK(expr) \
    do /* NOLINT(cppcoreguidelines-avoid-do-while) */ \
    { \
        _Pragma("GCC diagnostic push"); \
        _Pragma("GCC diagnostic ignored \"-Wparentheses\""); \
        _Pragma("GCC diagnostic ignored \"-Wunused\""); \
        _Pragma("GCC diagnostic ignored \"-Wunused-value\""); \
        /* NOLINTNEXTLINE(bugprone-chained-comparison) */ \
        (sts1cobcsw::AssertionDecomposer{"CHECK", #expr} < expr) == true; \
        _Pragma("GCC diagnostic pop"); \
    } while(false)

#define REQUIRE(expr) \
    do /* NOLINT(cppcoreguidelines-avoid-do-while) */ \
    { \
        _Pragma("GCC diagnostic push"); \
        _Pragma("GCC diagnostic ignored \"-Wparentheses\""); \
        _Pragma("GCC diagnostic ignored \"-Wunused\""); \
        /* NOLINTNEXTLINE(bugprone-chained-comparison) */ \
        if(not((sts1cobcsw::AssertionDecomposer{"REQUIRE", #expr} < expr) == true)) \
            return; \
        _Pragma("GCC diagnostic pop"); \
    } while(false)
// NOLINTEND(bugprone-macro-parentheses)
// NOLINTEND(cppcoreguidelines-macro-usage)
