#include <Sts1CobcSw/Utility/Span.hpp>

#include <catch2/catch_test_macros.hpp>

#include <etl/vector.h>

#include <algorithm>
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
    auto v = etl::vector<float, 3>{1.0, 2.0, 3.0};
    auto const cv = etl::vector<float, 3>{4.0, 5.0, 6.0};
    etl::ivector<float> & iv = v;
    etl::ivector<float> const & civ = cv;

    // Passing by const & yields std::span<T const> aka a read-only span
    STATIC_CHECK(std::is_same_v<decltype(Span(i)), std::span<int const, 1>>);
    STATIC_CHECK(std::is_same_v<decltype(Span(ci)), std::span<int const, 1>>);
    STATIC_CHECK(std::is_same_v<decltype(Span(123456L)), std::span<long const, 1>>);
    STATIC_CHECK(std::is_same_v<decltype(Span(a)), std::span<short const, 3>>);
    STATIC_CHECK(std::is_same_v<decltype(Span(ca)), std::span<char const, 5>>);
    STATIC_CHECK(std::is_same_v<decltype(Span("ijk")), std::span<char const, 4>>);
    STATIC_CHECK(std::is_same_v<decltype(Span({8U, 9U})), std::span<unsigned int const, 2>>);
    STATIC_CHECK(std::is_same_v<decltype(Span(sa)), std::span<short const, 3>>);
    STATIC_CHECK(std::is_same_v<decltype(Span(csa)), std::span<int const, 2>>);
    STATIC_CHECK(std::is_same_v<decltype(Span(v)), std::span<float const>>);
    STATIC_CHECK(std::is_same_v<decltype(Span(cv)), std::span<float const>>);
    STATIC_CHECK(std::is_same_v<decltype(Span(iv)), std::span<float const>>);
    STATIC_CHECK(std::is_same_v<decltype(Span(civ)), std::span<float const>>);

    // Passing a pointer yields std::span<T> aka a span with mutable elements
    STATIC_CHECK(std::is_same_v<decltype(Span(&i)), std::span<int, 1>>);
    STATIC_CHECK(std::is_same_v<decltype(Span(&a)), std::span<short, 3>>);
    STATIC_CHECK(std::is_same_v<decltype(Span(&sa)), std::span<short, 3>>);
    STATIC_CHECK(std::is_same_v<decltype(Span(&v)), std::span<float>>);
    STATIC_CHECK(std::is_same_v<decltype(Span(&iv)), std::span<float>>);

    // Passing a pointer to a constant object is not allowed aka does not compile
    STATIC_CHECK_FALSE(std::is_invocable_v<CallSpan, decltype(&ci)>);
    STATIC_CHECK_FALSE(std::is_invocable_v<CallSpan, decltype(&ca)>);
    STATIC_CHECK_FALSE(std::is_invocable_v<CallSpan, decltype(&"ijk")>);
    STATIC_CHECK_FALSE(std::is_invocable_v<CallSpan, decltype(&csa)>);
    STATIC_CHECK_FALSE(std::is_invocable_v<CallSpan, decltype(&cv)>);
    STATIC_CHECK_FALSE(std::is_invocable_v<CallSpan, decltype(&civ)>);
}
