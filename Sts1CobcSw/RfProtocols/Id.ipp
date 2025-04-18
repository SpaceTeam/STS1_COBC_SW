#pragma once


#include <Sts1CobcSw/RfProtocols/Id.hpp>


namespace sts1cobcsw
{
template<typename T, T... validIdValues>
template<T t>
    requires(((t == validIdValues) or ...))
constexpr auto Id<T, validIdValues...>::Make() -> Id
{
    return Id(std::integral_constant<T, t>{});
}


template<typename T, T... validIdValues>
constexpr auto Id<T, validIdValues...>::Make(T const & t) -> Result<Id>
{
    if(((t == validIdValues) or ...))
    {
        return Id(t);
    }
    return ErrorCode::invalidValue;
}


template<typename T, T... validIdValues>
constexpr auto Id<T, validIdValues...>::Value() const -> T
{
    return valueIsAnImplementationDetail;
}


template<typename T, T... validIdValues>
template<T t>
    requires(((t == validIdValues) or ...))  // NOLINTNEXTLINE(*named-parameter)
constexpr Id<T, validIdValues...>::Id(std::integral_constant<T, t>)
    : valueIsAnImplementationDetail(t)
{
}


template<typename T, T... validIdValues>
constexpr Id<T, validIdValues...>::Id(T const & t) : valueIsAnImplementationDetail(t)
{
}
}
