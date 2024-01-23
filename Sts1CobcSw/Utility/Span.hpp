//! @file
//! @brief  Convenient conversion functions to `std::span`
//!
//! If a function takes a `std::span` as parameter, use the different overloads of `Span()` provided
//! in this file to turn single objects or arrays of objects to the "correct" `std::span`.
//! Parameters passed by `& const` are converted to spans with read-only elements, and those passed
//! as a non-const pointer are converted to spans with mutable elements. This solves the problem
//! that `std::span<T>` does not implicitly convert to `std::span<T const>`.

#pragma once


#include <etl/vector.h>

#include <array>
#include <cstddef>  // for std::size_t
#include <span>
#include <type_traits>


namespace sts1cobcsw
{
template<typename T>
    requires(not std::is_pointer_v<T>)
[[nodiscard]] auto Span(T const & t) -> std::span<T const, 1>;

// The const & to pointer shenanigans is necessary to prevent array-to-pointer decay during template
// argument deduction
template<typename T>
    requires(not std::is_const_v<T>)
[[nodiscard]] auto Span(T * const & t) -> std::span<T, 1>;

template<typename T, std::size_t size>
[[nodiscard]] auto Span(T const (&t)[size]) -> std::span<T const, size>;

template<typename T, std::size_t size>
    requires(not std::is_const_v<T>)
[[nodiscard]] auto Span(T (*t)[size]) -> std::span<T, size>;

template<typename T, std::size_t size>
[[nodiscard]] auto Span(std::array<T, size> const & array) -> std::span<T const, size>;

template<typename T, std::size_t size>
[[nodiscard]] auto Span(std::array<T, size> * array) -> std::span<T, size>;

template<typename T, std::size_t size>
[[nodiscard]] auto Span(etl::vector<T, size> const & vector) -> std::span<T const>;

template<typename T, std::size_t size>
[[nodiscard]] auto Span(etl::vector<T, size> * vector) -> std::span<T>;
}


#include <Sts1CobcSw/Utility/Span.ipp>
