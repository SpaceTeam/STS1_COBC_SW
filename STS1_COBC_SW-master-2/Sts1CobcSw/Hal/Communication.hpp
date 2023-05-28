#pragma once


#include <Sts1CobcSw/Serial/Byte.hpp>

#include <rodos_no_using_namespace.h>

#include <etl/string.h>

#include <cstddef>
#include <span>
#include <string_view>


// TODO: Add declarations at the top to see all provided functionality at once
// TODO: Introduce convention write <-> message, read <-> answer
namespace sts1cobcsw::hal
{
using serial::Byte;


template<typename T, std::size_t size>
inline auto WriteTo(auto * communicationInterface, std::span<T, size> data)
{
    std::size_t nSentBytes = 0U;
    auto bytes = std::as_bytes(data);

    while(nSentBytes < bytes.size())
    {
        nSentBytes +=
            communicationInterface->write(bytes.data() + nSentBytes, bytes.size() - nSentBytes);
    }
}


inline auto WriteTo(auto * communicationInterface, std::string_view message)
{
    std::size_t nSentBytes = 0U;
    while(nSentBytes < message.size())
    {
        nSentBytes +=
            communicationInterface->write(message.data() + nSentBytes, message.size() - nSentBytes);
    }
}


template<std::size_t size>
[[nodiscard]] inline auto ReadFrom(auto * communicationInterface,
                                   std::span<std::byte, size> readBuffer)
{
    std::size_t nReadBytes = 0U;
    while(nReadBytes < size)
    {
        nReadBytes +=
            communicationInterface->read(data(readBuffer) + nReadBytes, size - nReadBytes);
    }
}


template<std::size_t size>
[[nodiscard]] inline auto ReadFrom(auto * communicationInterface, std::span<char, size> readBuffer)
{
    std::size_t nReadBytes = 0U;
    while(nReadBytes < size)
    {
        nReadBytes +=
            communicationInterface->read(data(readBuffer) + nReadBytes, size - nReadBytes);
    }
}


template<std::size_t size>
[[nodiscard]] inline auto WriteToReadFrom(auto * communicationInterface,
                                          std::string_view message,
                                          etl::string<size> * answer)
{
    answer->initialize_free_space();
    auto nReceivedBytes = communicationInterface->writeRead(
        message.data(), message.size(), answer->data(), answer->capacity());
    answer->trim_to_terminator();

    return nReceivedBytes;
}


// TODO: Try const correctness for span again
template<std::size_t nBytes>
auto WriteToReadFrom(auto * communicationInterface, std::span<Byte, nBytes> data)
    -> std::array<Byte, nBytes>
{
    auto readData = std::array<Byte, nBytes>{};
    communicationInterface->writeRead(std::data(data), nBytes, std::data(readData), nBytes);
    return readData;
}
}
