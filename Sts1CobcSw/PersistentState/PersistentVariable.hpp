#pragma once

#include <cstdint>

namespace sts1cobcsw
{
template<typename T, std::uint32_t address, std::uint32_t offset>
class PersistentVariable
{
public:
    PersistentVariable();

    auto Store(T t) -> void;
    auto Load() -> T;

private:
    T t1_ = T{};
    T t2_ = T{};
    T t3_ = T{};

    auto MajorityVote(T vote1, T vote2, T vote3) -> T;
};
}
