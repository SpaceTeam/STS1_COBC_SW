#include <Tests/CatchRodos/TestMacros.hpp>

#include <Sts1CobcSw/RfProtocols/Id.hpp>
#include <Sts1CobcSw/RfProtocols/IdCounters.hpp>

#include <cstdint>


using sts1cobcsw::Make;


namespace
{
struct MyIdFields
{
    std::uint8_t minor = 0;
    std::uint8_t major = 0;

    friend constexpr auto operator==(MyIdFields const &, MyIdFields const &) -> bool = default;
};

using MyId = sts1cobcsw::Id<MyIdFields, MyIdFields{0, 1}, MyIdFields{3, 5}, MyIdFields{7, 9}>;


auto myIdCounters = sts1cobcsw::IdCounters<int, MyId>{};

constexpr auto id1 = Make<MyId, MyIdFields{0, 1}>();
constexpr auto id2 = Make<MyId, MyIdFields{3, 5}>();
constexpr auto id3 = Make<MyId, MyIdFields{7, 9}>();
}


TEST_CASE("IdCounters")
{
    CHECK(myIdCounters.Get(id1) == 0);
    CHECK(myIdCounters.Get(id2) == 0);
    CHECK(myIdCounters.Get(id3) == 0);
    for(auto i = 0; i < 10; ++i)
    {
        CHECK(myIdCounters.PostIncrement(id1) == i);
        CHECK(myIdCounters.PostIncrement(id2) == i);
        CHECK(myIdCounters.PostIncrement(id3) == i);
        CHECK(myIdCounters.Get(id1) == i + 1);
        CHECK(myIdCounters.Get(id2) == i + 1);
        CHECK(myIdCounters.Get(id3) == i + 1);
    }
}
