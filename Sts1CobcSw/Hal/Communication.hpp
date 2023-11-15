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


template<typename T, std::size_t size>
auto WriteTo(auto * communicationInterface, std::span<T, size> data);

auto WriteTo(auto * communicationInterface, std::string_view message);

template<std::size_t size>
auto ReadFrom(auto * communicationInterface, std::span<std::byte, size> readBuffer);

template<std::size_t size>
auto ReadFrom(auto * communicationInterface, std::span<char, size> readBuffer);

template<std::size_t size>
auto WriteToReadFrom(auto * communicationInterface,
                     std::string_view message,
                     etl::string<size> * answer);

// TODO: Try const correctness for span again
template<std::size_t nBytes>
auto WriteToReadFrom(auto * communicationInterface, std::span<Byte, nBytes> data)
    -> std::array<Byte, nBytes>;
}


#include <Sts1CobcSw/Hal/Communication.ipp>
