#pragma once


#include <rodos/api/rodos-semaphore.h>

#include <optional>


namespace sts1cobcsw
{
template<typename T>
class EdacVariable
{
public:
    constexpr EdacVariable() = default;
    explicit constexpr EdacVariable(T const & value);
    EdacVariable(EdacVariable const &) = delete;
    EdacVariable(EdacVariable &&) = delete;
    auto operator=(EdacVariable const &) -> EdacVariable & = delete;
    auto operator=(EdacVariable &&) -> EdacVariable & = delete;
    ~EdacVariable() = default;

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


template<typename T>
[[nodiscard]] constexpr auto ComputeMajorityVote(T const & value0,
                                                 T const & value1,
                                                 T const & value2) -> std::optional<T>;
}


#include <Sts1CobcSw/Utility/ErrorDetectionAndCorrection.ipp>  // IWYU pragma: keep
