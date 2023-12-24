#pragma once


#include <Sts1CobcSw/Utility/Span.hpp>


namespace sts1cobcsw
{
template<typename T>
    requires(not std::is_pointer_v<T>)
inline auto Span(T const & t) -> std::span<T const, 1>
{
    return std::span<T const, 1>(&t, 1);
}


// The const & to pointer shenanigans is necessary to prevent array-to-pointer decay during template
// argument deduction
template<typename T>
    requires(not std::is_const_v<T>)
inline auto Span(T * const & t) -> std::span<T, 1>
{
    return std::span<T, 1>(t, 1);
}


template<typename T, std::size_t size>
inline auto Span(T const (&t)[size]) -> std::span<T const, size>
{
    return std::span<T const, size>(t);
}


template<typename T, std::size_t size>
    requires(not std::is_const_v<T>)
inline auto Span(T (*t)[size]) -> std::span<T, size>
{
    return std::span<T, size>(*t);
}


template<typename T, std::size_t size>
inline auto Span(std::array<T, size> const & array) -> std::span<T const, size>
{
    return std::span<T const, size>(array);
}


template<typename T, std::size_t size>
inline auto Span(std::array<T, size> * array) -> std::span<T, size>
{
    return std::span<T, size>(*array);
}
}
