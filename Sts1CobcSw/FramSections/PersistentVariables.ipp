#pragma once


#include <Sts1CobcSw/FramSections/PersistentVariables.hpp>
#include <Sts1CobcSw/Utility/ErrorDetectionAndCorrection.hpp>
#include <Sts1CobcSw/Utility/Span.hpp>


namespace sts1cobcsw
{
template<Section parentSection0,
         Section parentSection1,
         Section parentSection2,
         APersistentVariableInfo... PersistentVariableInfos>
    requires(sizeof...(PersistentVariableInfos) > 0 && parentSection0.end <= parentSection1.begin
             && parentSection1.end <= parentSection2.begin)
RODOS::Semaphore PersistentVariables<parentSection0,
                                     parentSection1,
                                     parentSection2,
                                     PersistentVariableInfos...>::semaphore = RODOS::Semaphore();


template<Section parentSection0,
         Section parentSection1,
         Section parentSection2,
         APersistentVariableInfo... PersistentVariableInfos>
    requires(sizeof...(PersistentVariableInfos) > 0 && parentSection0.end <= parentSection1.begin
             && parentSection1.end <= parentSection2.begin)
template<StringLiteral name>
auto PersistentVariables<parentSection0,
                         parentSection1,
                         parentSection2,
                         PersistentVariableInfos...>::Load() -> ValueType<name>
{
    auto protector = RODOS::ScopeProtector(&semaphore);  // NOLINT(google-readability-casting)
    auto [value0, value1, value2] =
        fram::framIsWorking.Load() ? ReadFromFram<name>() : ReadFromCache<name>();
    auto voteResult = ComputeMajorityVote(value0, value1, value2);
    auto value = voteResult.value_or(value0);
    auto allVotesAreEqual = (value0 == value1) && (value1 == value2);
    if(not allVotesAreEqual)
    {
        WriteToFram<name>(value);
    }
    WriteToCache<name>(value);
    return value;
}


template<Section parentSection0,
         Section parentSection1,
         Section parentSection2,
         APersistentVariableInfo... PersistentVariableInfos>
    requires(sizeof...(PersistentVariableInfos) > 0 && parentSection0.end <= parentSection1.begin
             && parentSection1.end <= parentSection2.begin)
template<StringLiteral name>
auto PersistentVariables<parentSection0,
                         parentSection1,
                         parentSection2,
                         PersistentVariableInfos...>::Store(ValueType<name> const & value)
{
    auto protector = RODOS::ScopeProtector(&semaphore);  // NOLINT(google-readability-casting)
    if(fram::framIsWorking.Load())
    {
        WriteToFram<name>(value);
    }
    WriteToCache<name>(value);
}


template<Section parentSection0,
         Section parentSection1,
         Section parentSection2,
         APersistentVariableInfo... PersistentVariableInfos>
    requires(sizeof...(PersistentVariableInfos) > 0 && parentSection0.end <= parentSection1.begin
             && parentSection1.end <= parentSection2.begin)
template<StringLiteral name>
auto PersistentVariables<parentSection0,
                         parentSection1,
                         parentSection2,
                         PersistentVariableInfos...>::ReadFromFram()
    -> std::array<ValueType<name>, 3>
{
    constexpr auto address0 = subsections0.template Get<name>().begin;
    constexpr auto address1 = subsections1.template Get<name>().begin;
    constexpr auto address2 = subsections2.template Get<name>().begin;
    constexpr auto size0 = value_of(subsections0.template Get<name>().size);
    constexpr auto size1 = value_of(subsections1.template Get<name>().size);
    constexpr auto size2 = value_of(subsections2.template Get<name>().size);
    auto value0 = Deserialize<ValueType<name>>(fram::ReadFrom<size0>(address0, spiTimeout));
    auto value1 = Deserialize<ValueType<name>>(fram::ReadFrom<size1>(address1, spiTimeout));
    auto value2 = Deserialize<ValueType<name>>(fram::ReadFrom<size2>(address2, spiTimeout));
    return {value0, value1, value2};
}


template<Section parentSection0,
         Section parentSection1,
         Section parentSection2,
         APersistentVariableInfo... PersistentVariableInfos>
    requires(sizeof...(PersistentVariableInfos) > 0 && parentSection0.end <= parentSection1.begin
             && parentSection1.end <= parentSection2.begin)
template<StringLiteral name>
auto PersistentVariables<parentSection0,
                         parentSection1,
                         parentSection2,
                         PersistentVariableInfos...>::WriteToFram(ValueType<name> const & value)
{
    constexpr auto address0 = subsections0.template Get<name>().begin;
    constexpr auto address1 = subsections1.template Get<name>().begin;
    constexpr auto address2 = subsections2.template Get<name>().begin;
    fram::WriteTo(address0, Span(Serialize(value)), spiTimeout);
    fram::WriteTo(address1, Span(Serialize(value)), spiTimeout);
    fram::WriteTo(address2, Span(Serialize(value)), spiTimeout);
}


template<Section parentSection0,
         Section parentSection1,
         Section parentSection2,
         APersistentVariableInfo... PersistentVariableInfos>
    requires(sizeof...(PersistentVariableInfos) > 0 && parentSection0.end <= parentSection1.begin
             && parentSection1.end <= parentSection2.begin)
template<StringLiteral name>
auto PersistentVariables<parentSection0,
                         parentSection1,
                         parentSection2,
                         PersistentVariableInfos...>::ReadFromCache()
    -> std::array<ValueType<name>, 3>
{
    constexpr auto index = subsections0.template Index<name>();
    return {get<index>(cache0), get<index>(cache1), get<index>(cache2)};
}


template<Section parentSection0,
         Section parentSection1,
         Section parentSection2,
         APersistentVariableInfo... PersistentVariableInfos>
    requires(sizeof...(PersistentVariableInfos) > 0 && parentSection0.end <= parentSection1.begin
             && parentSection1.end <= parentSection2.begin)
template<StringLiteral name>
auto PersistentVariables<parentSection0,
                         parentSection1,
                         parentSection2,
                         PersistentVariableInfos...>::WriteToCache(ValueType<name> const & value)
{
    constexpr auto index = subsections0.template Index<name>();
    get<index>(cache0) = value;
    get<index>(cache1) = value;
    get<index>(cache2) = value;
}


template<Section parentSection0,
         Section parentSection1,
         Section parentSection2,
         APersistentVariableInfo... PersistentVariableInfos>
    requires(sizeof...(PersistentVariableInfos) > 0 && parentSection0.end <= parentSection1.begin
             && parentSection1.end <= parentSection2.begin)
std::tuple<typename PersistentVariableInfos::ValueType...> PersistentVariables<
    parentSection0,
    parentSection1,
    parentSection2,
    PersistentVariableInfos...>::cache0 =
    std::tuple(typename PersistentVariableInfos::ValueType{}...);


template<Section parentSection0,
         Section parentSection1,
         Section parentSection2,
         APersistentVariableInfo... PersistentVariableInfos>
    requires(sizeof...(PersistentVariableInfos) > 0 && parentSection0.end <= parentSection1.begin
             && parentSection1.end <= parentSection2.begin)
std::tuple<typename PersistentVariableInfos::ValueType...> PersistentVariables<
    parentSection0,
    parentSection1,
    parentSection2,
    PersistentVariableInfos...>::cache1 =
    std::tuple(typename PersistentVariableInfos::ValueType{}...);


template<Section parentSection0,
         Section parentSection1,
         Section parentSection2,
         APersistentVariableInfo... PersistentVariableInfos>
    requires(sizeof...(PersistentVariableInfos) > 0 && parentSection0.end <= parentSection1.begin
             && parentSection1.end <= parentSection2.begin)
std::tuple<typename PersistentVariableInfos::ValueType...> PersistentVariables<
    parentSection0,
    parentSection1,
    parentSection2,
    PersistentVariableInfos...>::cache2 =
    std::tuple(typename PersistentVariableInfos::ValueType{}...);
}
