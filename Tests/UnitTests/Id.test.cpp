#include <Tests/CatchRodos/TestMacros.hpp>

#include <Sts1CobcSw/RfProtocols/Id.hpp>

#include <array>
#include <type_traits>


using sts1cobcsw::Make;
using MyId = sts1cobcsw::Id<int, 1, 2, 3, 17>;


// This struct allows us to use std::is_invocable_v to test if some calls to MyId::Make() do not
// compile without actually throwing compiler errors. See Span.test.cpp for an explanation.
struct CallMakeMyId
{
    template<int t>
    auto operator()(std::integral_constant<int, t>) const -> decltype(Make<MyId, t>());
};


// ValueType and validValues
static_assert(std::is_same_v<MyId::ValueType, int>);
static_assert(MyId::validValues == std::array{1, 2, 3, 17});

// Construction
static_assert(std::is_default_constructible_v<MyId>);
constexpr auto id0 = MyId();
constexpr auto id1 = MyId{1};
constexpr auto id5 = MyId(5);


// Make<>()
constexpr auto id2 = Make<MyId, 2>();
static_assert(std::is_invocable_v<CallMakeMyId, std::integral_constant<int, 3>>);
static_assert(std::is_invocable_v<CallMakeMyId, std::integral_constant<int, 17>>);
static_assert(not std::is_invocable_v<CallMakeMyId, std::integral_constant<int, -1>>);
static_assert(not std::is_invocable_v<CallMakeMyId, std::integral_constant<int, 0>>);
static_assert(not std::is_invocable_v<CallMakeMyId, std::integral_constant<int, 4>>);
static_assert(not std::is_invocable_v<CallMakeMyId, std::integral_constant<int, 16>>);
static_assert(not std::is_invocable_v<CallMakeMyId, std::integral_constant<int, 123'456>>);

// operator==()
static_assert(id1 == Make<MyId, 1>());
static_assert(id1 != id2);
static_assert(id1 != id5);

// Value()
static_assert(id0.Value() == 0);
static_assert(id1.Value() == 1);
static_assert(id5.Value() == 5);
static_assert(Make<MyId, 17>().Value() == 17);

// IsValid()
static_assert(not IsValid(id0));
static_assert(IsValid(id1));
static_assert(not IsValid(id5));


TEST_CASE("Id")
{
    auto myId = MyId{};
    CHECK(not IsValid(myId));
    myId = MyId(17);
    CHECK(IsValid(myId));
}
