#pragma once


#include <Sts1CobcSw/RfProtocols/Id.hpp>

#include <algorithm>


namespace sts1cobcsw
{
template<typename T, T... validIdValues>
    requires(sizeof...(validIdValues) > 0)
constexpr Id<T, validIdValues...>::Id(T t) : valueIsAnImplementationDetail(std::move(t))
{}


template<typename T, T... validIdValues>
    requires(sizeof...(validIdValues) > 0)
constexpr auto Id<T, validIdValues...>::Value() const -> T
{
    return valueIsAnImplementationDetail;
}


template<AnId Id>
[[nodiscard]] constexpr auto IsValid(Id const & id) -> bool
{
    return std::find(Id::validValues.begin(), Id::validValues.end(), id.Value())
        != Id::validValues.end();
}


template<AnId Id, typename Id::ValueType t>
    requires(IsValid(Id(t)))
[[nodiscard]] constexpr auto Make() -> Id
{
    return Id(t);
}


template<std::endian endianness, AnId Id>
auto SerializeTo(void * destination, Id const & id) -> void *
{
    return SerializeTo<endianness>(destination, id.Value());
}


template<std::endian endianness, AnId Id>
auto DeserializeFrom(void const * source, Id * id) -> void const *
{
    return DeserializeFrom<endianness>(source, &id->valueIsAnImplementationDetail);
}
}
