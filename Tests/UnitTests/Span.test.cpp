#include <Sts1CobcSw/Utility/Span.hpp>

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <span>
#include <type_traits>
#include <utility>


using sts1cobcsw::Span;


// The extra level of indirection provided by this function object is necessary to test if some
// calls to Span() do not compile without actually throwing compiler errors. The magic lies in
// decltype(expression) which only pretends to evaluate the expression to get its type. If the
// expression is ill-formed (i.e. it would not compile) the operator() for this type T is removed
// from the overload set and std::is_invocable_v<> returns false.
struct CallSpan
{
    template<typename T>
    auto operator()(T && t) const -> decltype(Span(std::forward<T>(t)));
};


TEST_CASE("Span() converts to the right std::span")
{
    auto i = 256;
    auto const ci = 17;
    short a[] = {1, 2, 3};  // NOLINT(misc-const-correctness)
    char const ca[] = "asdf";
    auto sa = std::array<short, 3>{2, 3, 4};
    auto const csa = std::array{0x12, 0x34};

    // Passing by const & yields std::span<T const> aka a read-only span
    CHECK(std::is_same_v<decltype(Span(i)), std::span<int const, 1>>);
    CHECK(std::is_same_v<decltype(Span(ci)), std::span<int const, 1>>);
    CHECK(std::is_same_v<decltype(Span(123456L)), std::span<long const, 1>>);
    CHECK(std::is_same_v<decltype(Span(a)), std::span<short const, 3>>);
    CHECK(std::is_same_v<decltype(Span(ca)), std::span<char const, 5>>);
    CHECK(std::is_same_v<decltype(Span("ijk")), std::span<char const, 4>>);
    CHECK(std::is_same_v<decltype(Span({8U, 9U})), std::span<unsigned int const, 2>>);
    CHECK(std::is_same_v<decltype(Span(sa)), std::span<short const, 3>>);
    CHECK(std::is_same_v<decltype(Span(csa)), std::span<int const, 2>>);

    // Passing a pointer yields std::span<T> aka a span with mutable elements
    CHECK(std::is_same_v<decltype(Span(&i)), std::span<int, 1>>);
    CHECK(std::is_same_v<decltype(Span(&a)), std::span<short, 3>>);
    CHECK(std::is_same_v<decltype(Span(&sa)), std::span<short, 3>>);

    // Passing a pointer to a constant object is not allowed aka does not compile
    CHECK_FALSE(std::is_invocable_v<CallSpan, decltype(&ci)>);
    CHECK_FALSE(std::is_invocable_v<CallSpan, decltype(&ca)>);
    CHECK_FALSE(std::is_invocable_v<CallSpan, decltype(&"ijk")>);
    CHECK_FALSE(std::is_invocable_v<CallSpan, decltype(&csa)>);
}
