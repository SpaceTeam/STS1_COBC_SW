#include <Sts1CobcSw/Utility/ErrorDetectionAndCorrection.hpp>

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cstring>
#include <optional>
#include <string>
#include <type_traits>


TEST_CASE("Majority vote")
{
    using sts1cobcsw::ComputeMajorityVote;

    auto voteResult = ComputeMajorityVote(173, 173, 173);
    CHECK(voteResult.has_value());
    CHECK(voteResult.value() == 173);  // NOLINT(*unchecked-optional-access)

    voteResult = ComputeMajorityVote(-2, 173, 173);
    CHECK(voteResult.has_value());
    CHECK(voteResult.value() == 173);  // NOLINT(*unchecked-optional-access)

    voteResult = ComputeMajorityVote(173, -2, 173);
    CHECK(voteResult.has_value());
    CHECK(voteResult.value() == 173);  // NOLINT(*unchecked-optional-access)

    voteResult = ComputeMajorityVote(173, 173, -2);
    CHECK(voteResult.has_value());
    CHECK(voteResult.value() == 173);  // NOLINT(*unchecked-optional-access)

    voteResult = ComputeMajorityVote(17, 173, -2);
    CHECK(not voteResult.has_value());
}


struct S
{
    char c = 's';

    [[nodiscard]] friend constexpr auto operator==(S const & lhs, S const & rhs) -> bool
    {
        return lhs.c == rhs.c;
    }
};


TEST_CASE("EdacVariable")
{
    using sts1cobcsw::EdacVariable;


    SECTION("Construction")
    {
        auto variable1 = EdacVariable<S>();
        auto variable2 = EdacVariable<int>();
        auto variable3 = EdacVariable<double>(-3.14);
        CHECK(variable1.Load().c == 's');
        CHECK(variable2.Load() == 0);
        CHECK(variable3.Load() == -3.14);
    }

    SECTION("You load what you store")
    {
        auto variable = EdacVariable<int>(17);
        CHECK(variable.Load() == 17);
        variable.Store(-123);
        CHECK(variable.Load() == -123);
    }

    SECTION("Error correction")
    {
        auto variable = EdacVariable<char>('a');
        CHECK(variable.Load() == 'a');

        // Those memcpy's are unspecified behavior but I can't think of a better way (= one that is
        // not undefined behavior) and it seems to work
        auto data = std::array<char, sizeof(variable)>{};
        std::memcpy(data.data(), &variable, sizeof(variable));
        data[0] = 'x';
        std::memcpy(&variable, data.data(), sizeof(variable));
        CHECK(variable.Load() == 'a');
        std::memcpy(data.data(), &variable, sizeof(variable));
        CHECK(data == std::array{'a', 'a', 'a'});

        data[1] = 'x';
        std::memcpy(&variable, data.data(), sizeof(variable));
        CHECK(variable.Load() == 'a');
        std::memcpy(data.data(), &variable, sizeof(variable));
        CHECK(data == std::array{'a', 'a', 'a'});

        data[2] = 'x';
        std::memcpy(&variable, data.data(), sizeof(variable));
        CHECK(variable.Load() == 'a');
        std::memcpy(data.data(), &variable, sizeof(variable));
        CHECK(data == std::array{'a', 'a', 'a'});

        data[0] = 'x';
        data[1] = 'x';
        std::memcpy(&variable, data.data(), sizeof(variable));
        CHECK(variable.Load() == 'x');
        std::memcpy(data.data(), &variable, sizeof(variable));
        CHECK(data == std::array{'x', 'x', 'x'});
    }
}
