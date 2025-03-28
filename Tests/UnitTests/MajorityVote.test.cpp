#include <Sts1CobcSw/Utility/ErrorDetectionAndCorrection.hpp>

#include <catch2/catch_test_macros.hpp>

#include <optional>


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
    CHECK(voteResult.has_value() == false);
}
