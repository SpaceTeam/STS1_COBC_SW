#pragma once


#include <Sts1CobcSw/FramSections/PersistentVariables.hpp>

#include <Sts1CobcSw/Utility/ErrorDetectionAndCorrection.hpp>
#include <Sts1CobcSw/Utility/Span.hpp>


namespace sts1cobcsw
{
template<Section section, APersistentVariableInfo... PersistentVariableInfos>
    requires(sizeof...(PersistentVariableInfos) > 0)
template<StringLiteral name>
auto PersistentVariables<section, PersistentVariableInfos...>::Load() -> ValueType<name>
{
    auto protector = RODOS::ScopeProtector(&semaphore);  // NOLINT(google-readability-casting)
    auto [value0, value1, value2] =
        fram::framIsWorking.Load() ? ReadFromFram<name>() : ReadFromCache<name>();
    auto voteResult = ComputeMajorityVote(value0, value1, value2);
    auto value = voteResult.value_or(value0);
    auto allVotesAreEqual = (value0 == value1) && (value1 == value2);
    if(not allVotesAreEqual and fram::framIsWorking.Load())
    {
        WriteToFram<name>(value);
    }
    WriteToCache<name>(value);
    return value;
}


template<Section section, APersistentVariableInfo... PersistentVariableInfos>
    requires(sizeof...(PersistentVariableInfos) > 0)
template<StringLiteral name>
auto PersistentVariables<section, PersistentVariableInfos...>::Store(ValueType<name> const & value)
    -> void
{
    auto protector = RODOS::ScopeProtector(&semaphore);  // NOLINT(google-readability-casting)
    if(fram::framIsWorking.Load())
    {
        WriteToFram<name>(value);
    }
    WriteToCache<name>(value);
}


template<Section section, APersistentVariableInfo... PersistentVariableInfos>
    requires(sizeof...(PersistentVariableInfos) > 0)
template<StringLiteral name>
auto PersistentVariables<section, PersistentVariableInfos...>::Increment() -> void
{
    auto protector = RODOS::ScopeProtector(&semaphore);  // NOLINT(google-readability-casting)
    auto [value0, value1, value2] =
        fram::framIsWorking.Load() ? ReadFromFram<name>() : ReadFromCache<name>();
    auto voteResult = ComputeMajorityVote(value0, value1, value2);
    auto value = voteResult.value_or(value0);
    value++;
    if(fram::framIsWorking.Load())
    {
        WriteToFram<name>(value);
    }
    WriteToCache<name>(value);
}


template<Section section, APersistentVariableInfo... PersistentVariableInfos>
    requires(sizeof...(PersistentVariableInfos) > 0)
template<StringLiteral name>
auto PersistentVariables<section, PersistentVariableInfos...>::Add(ValueType<name> const & value)
    -> void
{
    auto protector = RODOS::ScopeProtector(&semaphore);  // NOLINT(google-readability-casting)
    auto [value0, value1, value2] =
        fram::framIsWorking.Load() ? ReadFromFram<name>() : ReadFromCache<name>();
    auto voteResult = ComputeMajorityVote(value0, value1, value2);
    auto newValue = static_cast<ValueType<name>>(voteResult.value_or(value0) + value);
    if(fram::framIsWorking.Load())
    {
        WriteToFram<name>(newValue);
    }
    WriteToCache<name>(newValue);
}


template<Section section, APersistentVariableInfo... PersistentVariableInfos>
    requires(sizeof...(PersistentVariableInfos) > 0)
template<StringLiteral name>
auto PersistentVariables<section, PersistentVariableInfos...>::WriteToFram(
    ValueType<name> const & value) -> void
{
    constexpr auto address0 = variables0.template Get<name>().begin;
    constexpr auto address1 = variables1.template Get<name>().begin;
    constexpr auto address2 = variables2.template Get<name>().begin;
    fram::WriteTo(address0, Span(Serialize(value)), spiTimeout);
    fram::WriteTo(address1, Span(Serialize(value)), spiTimeout);
    fram::WriteTo(address2, Span(Serialize(value)), spiTimeout);
}


template<Section section, APersistentVariableInfo... PersistentVariableInfos>
    requires(sizeof...(PersistentVariableInfos) > 0)
template<StringLiteral name>
auto PersistentVariables<section, PersistentVariableInfos...>::WriteToCache(
    ValueType<name> const & value) -> void
{
    constexpr auto index = variables0.template Index<name>();
    get<index>(cache0) = value;
    get<index>(cache1) = value;
    get<index>(cache2) = value;
}


template<Section section, APersistentVariableInfo... PersistentVariableInfos>
    requires(sizeof...(PersistentVariableInfos) > 0)
template<StringLiteral name>
auto PersistentVariables<section, PersistentVariableInfos...>::ReadFromFram()
    -> std::array<ValueType<name>, 3>
{
    constexpr auto address0 = variables0.template Get<name>().begin;
    constexpr auto address1 = variables1.template Get<name>().begin;
    constexpr auto address2 = variables2.template Get<name>().begin;
    constexpr auto size0 = value_of(variables0.template Get<name>().size);
    constexpr auto size1 = value_of(variables1.template Get<name>().size);
    constexpr auto size2 = value_of(variables2.template Get<name>().size);
    auto value0 = Deserialize<ValueType<name>>(fram::ReadFrom<size0>(address0, spiTimeout));
    auto value1 = Deserialize<ValueType<name>>(fram::ReadFrom<size1>(address1, spiTimeout));
    auto value2 = Deserialize<ValueType<name>>(fram::ReadFrom<size2>(address2, spiTimeout));
    return {value0, value1, value2};
}


template<Section section, APersistentVariableInfo... PersistentVariableInfos>
    requires(sizeof...(PersistentVariableInfos) > 0)
template<StringLiteral name>
auto PersistentVariables<section, PersistentVariableInfos...>::ReadFromCache()
    -> std::array<ValueType<name>, 3>
{
    constexpr auto index = variables0.template Index<name>();
    return {get<index>(cache0), get<index>(cache1), get<index>(cache2)};
}


template<Section section, APersistentVariableInfo... PersistentVariableInfos>
    requires(sizeof...(PersistentVariableInfos) > 0)
std::tuple<typename PersistentVariableInfos::ValueType...>
    PersistentVariables<section, PersistentVariableInfos...>::cache0 =
        std::tuple(typename PersistentVariableInfos::ValueType{}...);


template<Section section, APersistentVariableInfo... PersistentVariableInfos>
    requires(sizeof...(PersistentVariableInfos) > 0)
std::tuple<typename PersistentVariableInfos::ValueType...>
    PersistentVariables<section, PersistentVariableInfos...>::cache1 =
        std::tuple(typename PersistentVariableInfos::ValueType{}...);


template<Section section, APersistentVariableInfo... PersistentVariableInfos>
    requires(sizeof...(PersistentVariableInfos) > 0)
std::tuple<typename PersistentVariableInfos::ValueType...>
    PersistentVariables<section, PersistentVariableInfos...>::cache2 =
        std::tuple(typename PersistentVariableInfos::ValueType{}...);


template<Section section, APersistentVariableInfo... PersistentVariableInfos>
    requires(sizeof...(PersistentVariableInfos) > 0)
RODOS::Semaphore PersistentVariables<section, PersistentVariableInfos...>::semaphore =
    RODOS::Semaphore();
}
