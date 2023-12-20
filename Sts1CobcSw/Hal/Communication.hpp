#pragma once


#include <Sts1CobcSw/Serial/Byte.hpp>

#include <rodos_no_using_namespace.h>

#include <etl/string.h>

#include <cstddef>
#include <span>
#include <string_view>


// TODO: Add declarations at the top to see all provided functionality at once
namespace sts1cobcsw::hal
{
// TODO: Remove extent to avoid code bload?
template<typename T, std::size_t extent>
auto WriteTo(auto * communicationInterface, std::span<T const, extent> data) -> void;

template<std::size_t extent>
auto ReadFrom(auto * communicationInterface, std::span<Byte, extent> data) -> void;

template<std::size_t size>
auto ReadFrom(RODOS::HAL_SPI * spi) -> std::array<Byte, size>;
}


#include <Sts1CobcSw/Hal/Communication.ipp>
