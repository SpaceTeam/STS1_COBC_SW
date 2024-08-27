#pragma once


#include <Sts1CobcSw/Utility/ErrorDetectionAndCorrection.hpp>


namespace sts1cobcsw
{
template<typename T>
RODOS::Semaphore EdacVariable<T>::semaphore{};


template<typename T>
constexpr EdacVariable<T>::EdacVariable(T const & value)
    : value0_(value), value1_(value), value2_(value)
{
}


template<typename T>
auto EdacVariable<T>::Load() const -> T
{
    auto protector = RODOS::ScopeProtector(&semaphore);  // NOLINT(google-readability-casting)
    auto voteResult = ComputeMajorityVote(value0_, value1_, value2_);
    auto value = voteResult.value_or(value0_);
    SetAllValues(value);
    return value;
}


template<typename T>
auto EdacVariable<T>::Store(T const & value) -> void
{
    auto protector = RODOS::ScopeProtector(&semaphore);  // NOLINT(google-readability-casting)
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
