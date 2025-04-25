#pragma once


#include <Sts1CobcSw/RfProtocols/Id.hpp>


namespace sts1cobcsw
{
template<typename T, T... validIdValues>
    requires(sizeof...(validIdValues) > 0)
constexpr Id<T, validIdValues...>::Id() : valueIsAnImplementationDetail(validValues[0])
{
}


template<typename T, T... validIdValues>
    requires(sizeof...(validIdValues) > 0)
template<T t>
    requires(((t == validIdValues) or ...))
constexpr auto Id<T, validIdValues...>::Make() -> Id
{
    return Id(std::integral_constant<T, t>{});
}


template<typename T, T... validIdValues>
    requires(sizeof...(validIdValues) > 0)
constexpr auto Id<T, validIdValues...>::Make(T const & t) -> Result<Id>
{
    if(((t == validIdValues) or ...))
    {
        return Id(t);
    }
    return ErrorCode::invalidValue;
}


template<typename T, T... validIdValues>
    requires(sizeof...(validIdValues) > 0)
constexpr auto Id<T, validIdValues...>::Value() const -> T
{
    return valueIsAnImplementationDetail;
}


template<typename T, T... validIdValues>
    requires(sizeof...(validIdValues) > 0)
template<T t>
    requires(((t == validIdValues) or ...))  // NOLINTNEXTLINE(*named-parameter)
constexpr Id<T, validIdValues...>::Id(std::integral_constant<T, t>)
    : valueIsAnImplementationDetail(t)
{
}


template<typename T, T... validIdValues>
    requires(sizeof...(validIdValues) > 0)
constexpr Id<T, validIdValues...>::Id(T const & t) : valueIsAnImplementationDetail(t)
{
}
}
