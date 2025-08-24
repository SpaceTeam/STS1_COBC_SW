#pragma once

#ifndef BUILD_BOOTLOADER
    #include <rodos/api/rodos-semaphore.h>


namespace sts1cobcsw
{
template<typename T>
class EdacVariable
{
public:
    constexpr EdacVariable() = default;
    explicit constexpr EdacVariable(T const & value);

    [[nodiscard]] auto Load() const -> T;
    auto Store(T const & value) -> void;


private:
    // This function must be const, because it is called from Load() which is const
    constexpr auto SetAllValues(T const & value) const -> void;

    static RODOS::Semaphore semaphore;

    mutable T value0_ = T{};
    mutable T value1_ = T{};
    mutable T value2_ = T{};
};
}


    #include <Sts1CobcSw/ErrorDetectionAndCorrection/EdacVariable.ipp>  // IWYU pragma: keep
#endif
