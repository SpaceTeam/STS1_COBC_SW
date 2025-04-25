#include <Tests/CatchRodos/TestMacros.hpp>

#include <Sts1CobcSw/Outcome/Outcome.hpp>
#include <Sts1CobcSw/RfProtocols/Id.hpp>

#include <algorithm>
#include <array>
#include <initializer_list>
#include <type_traits>


using MyId = sts1cobcsw::Id<int, 1, 2, 3, 17>;


// This struct allows us to use std::is_invocable_v to test if some calls to MyId::Make() do not
// compile without actually throwing compiler errors. See Span.test.cpp for an explanation.
struct CallMyIdMake
{
    template<int t>
    auto operator()(std::integral_constant<int, t>) const -> decltype(MyId::Make<t>());
};


// validValues
static_assert(MyId::validValues == std::array{1, 2, 3, 17});

// Make<>()
static constexpr auto id1 = MyId::Make<1>();
static constexpr auto id2 = MyId::Make<2>();
static_assert(std::is_invocable_v<CallMyIdMake, std::integral_constant<int, 3>>);
static_assert(std::is_invocable_v<CallMyIdMake, std::integral_constant<int, 17>>);
static_assert(not std::is_invocable_v<CallMyIdMake, std::integral_constant<int, -1>>);
static_assert(not std::is_invocable_v<CallMyIdMake, std::integral_constant<int, 0>>);
static_assert(not std::is_invocable_v<CallMyIdMake, std::integral_constant<int, 4>>);
static_assert(not std::is_invocable_v<CallMyIdMake, std::integral_constant<int, 16>>);
static_assert(not std::is_invocable_v<CallMyIdMake, std::integral_constant<int, 123456>>);

// operator==()
static_assert(id1 == MyId::Make<1>());
static_assert(id1 != id2);

// Value()
static_assert(id1.Value() == 1);
static_assert(id2.Value() == 2);
static_assert(MyId::Make<17>().Value() == 17);

// Default constructor
static_assert(std::is_default_constructible_v<MyId>);
static_assert(MyId{} == MyId::Make<1>());
static_assert(MyId{}.Value() == 1);


TEST_CASE("Id")
{
    auto makeResult = MyId::Make(1);
    CHECK(makeResult.has_value());
    CHECK(makeResult.value() == MyId::Make<1>());

    for(auto i : MyId::validValues)
    {
        makeResult = MyId::Make(i);
        CHECK(makeResult.has_value());
        auto myId = makeResult.value();
        CHECK(myId.Value() == i);
    }

    for(auto i : {-1, 0, 16, 123456})
    {
        makeResult = MyId::Make(i);
        CHECK(makeResult.has_error());
        CHECK(makeResult.error() == sts1cobcsw::ErrorCode::invalidValue);
    }
}
