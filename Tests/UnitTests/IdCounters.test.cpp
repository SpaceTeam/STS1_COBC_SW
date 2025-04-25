#include <Tests/CatchRodos/TestMacros.hpp>

#include <Sts1CobcSw/RfProtocols/Id.hpp>
#include <Sts1CobcSw/RfProtocols/IdCounters.hpp>


using MyId = sts1cobcsw::Id<int, 1, 4, 7>;


auto myIdCounters = sts1cobcsw::IdCounters<int, MyId>{};

constexpr auto id1 = MyId::Make<1>();
constexpr auto id4 = MyId::Make<4>();
constexpr auto id7 = MyId::Make<7>();


TEST_CASE("IdCounters")
{
    CHECK(myIdCounters.Get(id1) == 0);
    CHECK(myIdCounters.Get(id4) == 0);
    CHECK(myIdCounters.Get(id7) == 0);
    for(auto i = 0; i < 10; ++i)
    {
        CHECK(myIdCounters.PostIncrement(id1) == i);
        CHECK(myIdCounters.PostIncrement(id4) == i);
        CHECK(myIdCounters.PostIncrement(id7) == i);
        CHECK(myIdCounters.Get(id1) == i + 1);
        CHECK(myIdCounters.Get(id4) == i + 1);
        CHECK(myIdCounters.Get(id7) == i + 1);
    }
}
