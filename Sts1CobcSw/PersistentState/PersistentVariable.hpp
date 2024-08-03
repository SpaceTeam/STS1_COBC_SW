#pragma once


#include <Sts1CobcSw/Periphery/Fram.hpp>

#include <cstdint>


namespace sts1cobcsw
{
template<typename T, fram::Address address, fram::Size offset>
class PersistentVariable
{
public:
    PersistentVariable();

    auto Store(T const & t) -> void;
    auto Load() -> T;


private:
    T t1_ = T{};
    T t2_ = T{};
    T t3_ = T{};

    auto MajorityVote(T const & vote1, T const & vote2, T const & vote3) -> T;
};
}


#include <Sts1CobcSw/PersistentState/PersistentVariable.ipp>  // IWYU pragma: keep
