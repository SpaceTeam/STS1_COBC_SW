#pragma once


#include <Sts1CobcSw/Serial/Serial.hpp>

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>


namespace sts1cobcsw
{
namespace eps
{
using AdcValue = std::uint16_t;

inline constexpr auto nChannels = 16U;


struct AdcData
{
    std::array<AdcValue, nChannels> adc4;
    std::array<AdcValue, 10> adc5;  // NOLINT(*magic-numbers)
    std::array<AdcValue, 10> adc6;  // NOLINT(*magic-numbers)

    friend auto operator==(AdcData const &, AdcData const &) -> bool = default;
};


auto InitializeAdcs() -> void;
[[nodiscard]] auto ReadAdcs() -> AdcData;
auto ResetAdcRegisters() -> void;
auto ClearAdcFifos() -> void;

template<std::endian endianness>
[[nodiscard]] auto DeserializeFrom(void const * source, AdcData * data) -> void const *;
template<std::endian endianness>
[[nodiscard]] auto SerializeTo(void * destination, AdcData const & data) -> void *;
}


template<>
inline constexpr std::size_t serialSize<eps::AdcData> =
    totalSerialSize<decltype(eps::AdcData::adc4),
                    decltype(eps::AdcData::adc5),
                    decltype(eps::AdcData::adc6)>;
}


#include <Sts1CobcSw/Periphery/Eps.ipp>  // IWYU pragma: keep
