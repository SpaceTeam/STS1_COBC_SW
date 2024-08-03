#pragma once


#include <Sts1CobcSw/Periphery/Fram.hpp>
#include <Sts1CobcSw/Utility/StringLiteral.hpp>


namespace sts1cobcsw::fram
{
template<StringLiteral subsectionName, Size subsectionSize>
    requires(subsectionSize > 0)
struct SubsectionInfo
{
    static constexpr auto name = subsectionName;
    static constexpr Size size = subsectionSize;
};


template<typename T>
concept SubsectionInfoLike = std::derived_from<T, SubsectionInfo<T::name, T::size>>;


template<SubsectionInfoLike T, SubsectionInfoLike... Ts>
// NOLINTNEXTLINE(misc-redundant-expression)
inline constexpr auto nameAppearsOnce = (((T::name == Ts::name) ? 1 : 0) + ...) == 1;

template<SubsectionInfoLike... Ts>
inline constexpr auto containsNoDuplicateNames = (nameAppearsOnce<Ts, Ts...> && ...);
}
