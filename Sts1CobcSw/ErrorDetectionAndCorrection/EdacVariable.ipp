#pragma once

#ifndef BUILD_BOOTLOADER
    #include <Sts1CobcSw/ErrorDetectionAndCorrection/EdacVariable.hpp>

    #include <Sts1CobcSw/ErrorDetectionAndCorrection/ErrorDetectionAndCorrection.hpp>
    #include <Sts1CobcSw/Serial/Serial.hpp>
    #include <Sts1CobcSw/Utility/Span.hpp>


namespace sts1cobcsw
{
template<typename T>
RODOS::Semaphore EdacVariable<T>::semaphore{};


template<typename T>
constexpr EdacVariable<T>::EdacVariable(T const & value)
    : value0_(value), value1_(value), value2_(value)
{}


template<typename T>
auto EdacVariable<T>::Load() const -> T
{
    auto protector = RODOS::ScopeProtector(&semaphore);  // NOLINT(google-readability-casting)
    auto value = Deserialize<T>(ComputeBitwiseMajorityVote(
        Span(Serialize(value0_)), Span(Serialize(value1_)), Span(Serialize(value2_))));
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
}
#endif
