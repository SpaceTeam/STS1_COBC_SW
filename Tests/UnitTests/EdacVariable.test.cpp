#include <Tests/CatchRodos/TestMacros.hpp>

#include <Sts1CobcSw/Utility/ErrorDetectionAndCorrection.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>


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


TEST_CASE("EdacVariable")
{
    using sts1cobcsw::EdacVariable;

    // Construction
    {
        auto variable1 = EdacVariable<S>();
        auto variable2 = EdacVariable<int>();
        auto variable3 = EdacVariable<double>(-3.14);
        CHECK(variable1.Load().c == 's');
        CHECK(variable2.Load() == 0);
        CHECK(ApproximatelyEqual(variable3.Load(), -3.14));
    }

    // You load what you store
    {
        auto variable = EdacVariable<int>(17);
        CHECK(variable.Load() == 17);
        variable.Store(-123);
        CHECK(variable.Load() == -123);
    }

    // Error correction
    {
        auto variable = EdacVariable<char>('a');
        CHECK(variable.Load() == 'a');

        auto data = std::array<char, sizeof(variable)>{};
        std::memcpy(data.data(), &variable, sizeof(variable));
        data[0] = 'x';
        std::memcpy(&variable, data.data(), sizeof(variable));
        CHECK(variable.Load() == 'a');
        std::memcpy(data.data(), &variable, sizeof(variable));
        CHECK(data == (std::array{'a', 'a', 'a'}));

        data[1] = 'x';
        std::memcpy(&variable, data.data(), sizeof(variable));
        CHECK(variable.Load() == 'a');
        std::memcpy(data.data(), &variable, sizeof(variable));
        CHECK(data == (std::array{'a', 'a', 'a'}));

        data[2] = 'x';
        std::memcpy(&variable, data.data(), sizeof(variable));
        CHECK(variable.Load() == 'a');
        std::memcpy(data.data(), &variable, sizeof(variable));
        CHECK(data == (std::array{'a', 'a', 'a'}));

        data[0] = 'x';
        data[1] = 'x';
        std::memcpy(&variable, data.data(), sizeof(variable));
        CHECK(variable.Load() == 'x');
        std::memcpy(data.data(), &variable, sizeof(variable));
        CHECK(data == (std::array{'x', 'x', 'x'}));
    }
}
