#pragma once


#include <Sts1CobcSw/Periphery/SubsectionInfo.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>
#include <Sts1CobcSw/Utility/StringLiteral.hpp>


namespace sts1cobcsw::fram
{
template<StringLiteral variableName, typename T>
    requires(serialSize<T> > 0)
struct PersistentVariableInfo : SubsectionInfo<variableName, Size(serialSize<T>)>
{
    using ValueType = T;
};


namespace internal
{
template<typename T>
inline constexpr auto isAPersistentVariableInfoHelper = false;

template<StringLiteral name, typename T>
inline constexpr auto isAPersistentVariableInfoHelper<PersistentVariableInfo<name, T>> = true;
}


template<typename T>
inline constexpr bool isAPersistentVariableInfo =
    internal::isAPersistentVariableInfoHelper<std::remove_cvref_t<T>>;


template<typename T>
concept APersistentVariableInfo = isAPersistentVariableInfo<T>;
}
