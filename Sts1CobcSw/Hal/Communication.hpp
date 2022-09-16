#pragma once

#include <rodos.h>

#include <etl/string.h>

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <type_traits>


namespace sts1cobcsw::hal
{
template<typename T>
concept Writable = requires(T t, void const * sendBuf, std::size_t len)
{
    // TODO: Check why clang-format fucks this up
    {
        t.write(sendBuf, len)
        } -> std::integral;
};


template<typename T>
concept ReadWritable =
    requires(T t, const void * sendBuf, std::size_t len, void * recBuf, std::size_t maxLen)
{
    {
        t.writeRead(sendBuf, len, recBuf, maxLen)
        } -> std::integral;
};


template<typename T, std::size_t size>
inline auto WriteTo(Writable auto * communicationInterface, std::span<T, size> data)
{
    std::size_t nSentBytes = 0;
    auto bytes = std::as_bytes(data);

    while(nSentBytes < bytes.size())
    {
        nSentBytes +=
            communicationInterface->write(bytes.data() + nSentBytes, bytes.size() - nSentBytes);
    }
}


inline auto WriteTo(Writable auto * communicationInterface, std::string_view message)
{
    std::size_t nSentBytes = 0;
    while(nSentBytes < message.size())
    {
        nSentBytes +=
            communicationInterface->write(message.data() + nSentBytes, message.size() - nSentBytes);
    }
}


template<std::size_t size>
inline auto WriteToReadFrom(ReadWritable auto * communicationInterface,
                            std::string_view message,
                            etl::string<size> * answer)
{
    answer->initialize_free_space();
    auto nReceivedBytes = communicationInterface->writeRead(
        message.data(), message.size(), answer->data(), answer->capacity());
    answer->trim_to_terminator();

    return nReceivedBytes;
}
}
