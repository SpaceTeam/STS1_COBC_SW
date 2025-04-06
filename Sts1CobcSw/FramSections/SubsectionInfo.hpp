#pragma once


#include <Sts1CobcSw/Fram/Fram.hpp>
#include <Sts1CobcSw/Utility/StringLiteral.hpp>


namespace sts1cobcsw
{
template<StringLiteral subsectionName, fram::Size subsectionSize>
    requires(subsectionSize > 0)
struct SubsectionInfo
{
    static constexpr auto name = subsectionName;
    static constexpr auto size = subsectionSize;
};


template<typename T>
concept SubsectionInfoLike = std::derived_from<T, SubsectionInfo<T::name, T::size>>;

template<SubsectionInfoLike T, SubsectionInfoLike... Ts>
// NOLINTNEXTLINE(misc-redundant-expression)
inline constexpr auto nameAppearsOnce = (((T::name == Ts::name) ? 1 : 0) + ...) == 1;

template<SubsectionInfoLike... Ts>
inline constexpr auto containsNoDuplicateNames = (nameAppearsOnce<Ts, Ts...> && ...);
}
