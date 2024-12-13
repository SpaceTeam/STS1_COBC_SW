#include <Tests/UnitTests/UnitTestThread.hpp>

#include <Sts1CobcSw/Utility/ErrorDetectionAndCorrection.hpp>

#include <array>
#include <cmath>
#include <cstring>
#include <type_traits>


struct S
{
    char c = 's';

    [[nodiscard]] friend constexpr auto operator==(S const & lhs, S const & rhs) -> bool
    {
        return lhs.c == rhs.c;
    }
};


auto ApproximatelyEqual(double lhs, double rhs, double relativeTolerance = 1e-6) -> bool
{
    return std::abs(lhs - rhs) < relativeTolerance * std::max(std::abs(lhs), std::abs(rhs));
}


auto RunUnitTest() -> void
{
    using sts1cobcsw::EdacVariable;

    // SECTION("Construction")
    {
        auto variable1 = EdacVariable<S>();
        auto variable2 = EdacVariable<int>();
        auto variable3 = EdacVariable<double>(-3.14);
        Require(variable1.Load().c == 's');
        Require(variable2.Load() == 0);
        Require(ApproximatelyEqual(variable3.Load(), -3.14));
    }

    // SECTION("You load what you store")
    {
        auto variable = EdacVariable<int>(17);
        Require(variable.Load() == 17);
        variable.Store(-123);
        Require(variable.Load() == -123);
    }

    // SECTION("Error correction")
    {
        auto variable = EdacVariable<char>('a');
        Require(variable.Load() == 'a');

        // Those memcpy's are unspecified behavior but I can't think of a better way (= one that is
        // not undefined behavior) and it seems to work
        auto data = std::array<char, sizeof(variable)>{};
        std::memcpy(data.data(), &variable, sizeof(variable));
        data[0] = 'x';
        std::memcpy(&variable, data.data(), sizeof(variable));
        Require(variable.Load() == 'a');
        std::memcpy(data.data(), &variable, sizeof(variable));
        Require(data == std::array{'a', 'a', 'a'});

        data[1] = 'x';
        std::memcpy(&variable, data.data(), sizeof(variable));
        Require(variable.Load() == 'a');
        std::memcpy(data.data(), &variable, sizeof(variable));
        Require(data == std::array{'a', 'a', 'a'});

        data[2] = 'x';
        std::memcpy(&variable, data.data(), sizeof(variable));
        Require(variable.Load() == 'a');
        std::memcpy(data.data(), &variable, sizeof(variable));
        Require(data == std::array{'a', 'a', 'a'});

        data[0] = 'x';
        data[1] = 'x';
        std::memcpy(&variable, data.data(), sizeof(variable));
        Require(variable.Load() == 'x');
        std::memcpy(data.data(), &variable, sizeof(variable));
        Require(data == std::array{'x', 'x', 'x'});
    }
}
