#pragma once


#include <Sts1CobcSw/Utility/ErrorDetectionAndCorrection.hpp>


namespace sts1cobcsw
{
template<typename T>
constexpr EdacVariable<T>::EdacVariable(T const & value)
    : value0_(value), value1_(value), value2_(value)
{
}


template<typename T>
constexpr auto EdacVariable<T>::Load() const -> T
{
    // TODO: Make Load() thread-safe/atomic
    auto voteResult = ComputeMajorityVote(value0_, value1_, value2_);
    auto value = voteResult.value_or(value0_);
    SetAllValues(value);
    return value;
}


template<typename T>
constexpr auto EdacVariable<T>::Store(T const & value) -> void
{
    // TODO: Make Store() thread-safe/atomic
    SetAllValues(value);
}


template<typename T>
constexpr auto EdacVariable<T>::SetAllValues(T const & value) const -> void
{
    value0_ = value;
    value1_ = value;
    value2_ = value;
}


template<typename T>
constexpr auto ComputeMajorityVote(T const & value0, T const & value1, T const & value2)
    -> std::optional<T>
{
    if(value0 == value1 or value0 == value2)
    {
        return value0;
    }
    if(value1 == value2)
    {
        return value1;
    }
    return std::nullopt;
}
}
