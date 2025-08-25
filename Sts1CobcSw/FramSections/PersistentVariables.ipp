#pragma once


#include <Sts1CobcSw/FramSections/PersistentVariables.hpp>

#include <Sts1CobcSw/ErrorDetectionAndCorrection/ErrorDetectionAndCorrection.hpp>
#include <Sts1CobcSw/Utility/Span.hpp>
#include <Sts1CobcSw/Vocabulary/Ids.hpp>


namespace sts1cobcsw
{
template<Section section, APersistentVariableInfo... PersistentVariableInfos>
    requires(sizeof...(PersistentVariableInfos) > 0)
template<StringLiteral name>
auto PersistentVariables<section, PersistentVariableInfos...>::Load() -> ValueType<name>
{
    auto protector = RODOS::ScopeProtector(&semaphore);  // NOLINT(google-readability-casting)
    auto value = LoadValue<name>();
    if(fram::framIsWorking.Load())
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
    auto value = LoadValue<name>();
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
    auto oldValue = LoadValue<name>();
    auto newValue = static_cast<ValueType<name>>(oldValue + value);
    if(fram::framIsWorking.Load())
    {
        WriteToFram<name>(newValue);
    }
    WriteToCache<name>(newValue);
}


// TODO: Look at this function and the special treatment of PartitionId again, once we use LTO and
// the binary is still too large. At the time of writing this comment, using LoadValue() alone
// increases the binary size by 1100–1200 B and the special treatment of PartitionId adds another
// 130–180 B.
template<Section section, APersistentVariableInfo... PersistentVariableInfos>
    requires(sizeof...(PersistentVariableInfos) > 0)
template<StringLiteral name>
auto PersistentVariables<section, PersistentVariableInfos...>::LoadValue() -> ValueType<name>
{
    auto data = fram::framIsWorking.Load() ? ReadFromFram<name>() : ReadFromCache<name>();
    if constexpr(std::is_same_v<ValueType<name>, PartitionId>)
    {
        data[0][0] = ToClosestSecondaryPartitionId(data[0]);
        data[1][0] = ToClosestSecondaryPartitionId(data[1]);
        data[2][0] = ToClosestSecondaryPartitionId(data[2]);
    }
    return Deserialize<ValueType<name>>(
        ComputeBitwiseMajorityVote(Span(data[0]), Span(data[1]), Span(data[2])));
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
    -> std::array<SerialBuffer<ValueType<name>>, 3>
{
    constexpr auto address0 = variables0.template Get<name>().begin;
    constexpr auto address1 = variables1.template Get<name>().begin;
    constexpr auto address2 = variables2.template Get<name>().begin;
    constexpr auto size0 = value_of(variables0.template Get<name>().size);
    constexpr auto size1 = value_of(variables1.template Get<name>().size);
    constexpr auto size2 = value_of(variables2.template Get<name>().size);
    auto data0 = fram::ReadFrom<size0>(address0, spiTimeout);
    auto data1 = fram::ReadFrom<size1>(address1, spiTimeout);
    auto data2 = fram::ReadFrom<size2>(address2, spiTimeout);
    return {data0, data1, data2};
}


template<Section section, APersistentVariableInfo... PersistentVariableInfos>
    requires(sizeof...(PersistentVariableInfos) > 0)
template<StringLiteral name>
auto PersistentVariables<section, PersistentVariableInfos...>::ReadFromCache()
    -> std::array<SerialBuffer<ValueType<name>>, 3>
{
    constexpr auto index = variables0.template Index<name>();
    return {Serialize(get<index>(cache0)),
            Serialize(get<index>(cache1)),
            Serialize(get<index>(cache2))};
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
