#pragma once


#include <etl/string.h>

#include <type_traits>


// I am too lazy to add the whole GSL just for this one type alias
namespace gsl
{
using czstring = char const *;  // NOLINT(*identifier-naming)
}


namespace sts1cobcsw
{
static constexpr auto maxValueStringLength = 1000;
using ValueString = etl::string<maxValueStringLength>;


template<typename T>
concept Number =
    std::is_arithmetic_v<T> and not(std::is_same_v<T, bool> or std::is_same_v<T, char>);

template<typename T>
concept SignedNumber = Number<T> and std::is_signed_v<T>;

template<typename T>
concept UnsignedNumber = Number<T> and std::is_unsigned_v<T>;


static_assert(not Number<bool>);
static_assert(not Number<char>);
static_assert(not Number<std::byte>);

static_assert(SignedNumber<signed char> and not UnsignedNumber<signed char>);
static_assert(SignedNumber<short> and not UnsignedNumber<short>);
static_assert(SignedNumber<int> and not UnsignedNumber<int>);
static_assert(SignedNumber<long> and not UnsignedNumber<long>);
static_assert(SignedNumber<long long> and not UnsignedNumber<long long>);
static_assert(SignedNumber<float> and not UnsignedNumber<float>);
static_assert(SignedNumber<double> and not UnsignedNumber<double>);
static_assert(SignedNumber<long double> and not UnsignedNumber<long double>);

static_assert(UnsignedNumber<unsigned char> and not SignedNumber<unsigned char>);
static_assert(UnsignedNumber<unsigned short> and not SignedNumber<unsigned short>);
static_assert(UnsignedNumber<unsigned int> and not SignedNumber<unsigned int>);
static_assert(UnsignedNumber<unsigned long> and not SignedNumber<unsigned long>);
static_assert(UnsignedNumber<unsigned long long> and not SignedNumber<unsigned long long>);
}
