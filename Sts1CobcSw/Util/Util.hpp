#pragma once

#include <type_safe/types.hpp>

#include <etl/string.h>

#include <cstdint>
#include <cstring>
#include <span>
#include <vector>

namespace sts1cobcsw::util
{
namespace ts = type_safe;

/**
 * @brief Converts two bytes into a uint16_t
 *
 * @param msb The most significant byte
 * @param lsb The least significant byte
 *
 * @returns The combined uint16_t
 */
auto BytesToUint16(uint8_t msb, uint8_t lsb) -> uint16_t;

/**
 * @brief Converts two bytes into a uint16_t
 *
 * @param firstByte The most significant byte
 * @param secondByte The second most significant byte
 * @param secondByte The third most significant byte
 * @param fourthByte The least significant byte
 *
 * @returns The combined uint32_t
 */
auto BytesToUint32(uint8_t firstByte, uint8_t secondByte, uint8_t thirdByte, uint8_t fourthByte)
    -> uint32_t;

/**
 * @brief Converts a sequence of uint32_t elements to a uint8_t vector
 *
 * For some functions, in particular to calculate the CRC32 checksum,
 * arrays have to be converted to bytes. This function splits
 * 32 bit values with bit operations and writes the result into a vector.
 *
 * @param src The source sequence
 *
 * @returns The uint8_t vector
 */
auto VecUint32ToUint8(std::span<uint32_t> src) -> std::vector<uint8_t>;

/**
 * @brief Converts a sequence of uint16_t elements to a uint8_t vector
 *
 * For some functions, in particular to calculate the CRC32 checksum,
 * arrays have to be converted to bytes. This function splits
 * 16 bit values with bit operations and writes the result into a vector.
 *
 * @param src The source sequence
 *
 * @returns The uint8_t vector
 */
auto VecUint16ToUint8(std::span<uint16_t> src) -> std::vector<uint8_t>;

/**
 * @brief Implementation of the CRC32/MPEG-2 algorithm.
 *
 * https://en.wikipedia.org/wiki/Cyclic_redundancy_check  -> What is CRC
 * https://reveng.sourceforge.io/crc-catalogue/all.htm    -> Description of MPEG-2 implementation
 * https://docs.rs/crc/3.0.0/src/crc/crc32.rs.html        -> Rust implementation on the EDU side
 * https://gist.github.com/Miliox/b86b60b9755faf3bd7cf    -> C++ implementation of MPEG-2
 * https://crccalc.com/                                   -> To check the implementation
 *
 * @param data The data over which the checksum is calculated
 *
 * @returns The 32 bit checksum
 */
auto Crc32(std::span<uint8_t> data) -> uint32_t;

/**
 * @brief Copy a variable to a buffer.
 *
 * During the process, the position parameter is updated, so that one can chain
 * multiple calls to CopyTo(). The size of the variable that must be copied from
 * the buffer is the size of the value parameter.
 *
 * @param buffer  The buffer our data is copied to.
 * @param position The position in the buffer our data is copied to.
 * @param value The value to be copied to the buffer.
 */
auto CopyTo(std::span<std::byte> buffer, ts::size_t * const position, auto value)
{
    auto newPosition = *position + sizeof(value);
    std::memcpy(&buffer[(*position).get()], &value, sizeof(value));
    *position = newPosition;
}

/**
 * @brief Copy a value from a buffer to a variable.
 *
 * During the process, the position parameter is updated, so that one can chain
 * multiple calls to CopyFrom(). The size of the variable that must be copied from
 * the buffer is the size of the value parameter.
 *
 * @param buffer The buffer our data is copied from.
 * @param position The position in the buffer our data is copied from.
 * @param value The variable that will hold our copied value.
 */
template<std::size_t size>
auto CopyFrom(etl::string<size> const & buffer, ts::size_t * const position, auto * value)
{
    auto newPosition = *position + sizeof(*value);
    std::memcpy(value, &buffer[(*position).get()], sizeof(*value));
    *position = newPosition;
}

// TODO: documentation
auto CopyFrom(std::span<std::byte> buffer, ts::size_t * const position, auto * value)
{
    auto newPosition = *position + sizeof(*value);
    std::memcpy(value, &buffer[(*position).get()], sizeof(*value));
    *position = newPosition;
}



/*
 * @brief A concept to ensure an object is writable
 *
 * For RODOS, this applies to HAL_UART and HAL_SPI classes.
 */
template<typename T>
concept Writable = requires(T t, void const * sendBuf, std::size_t len)
{
    // TODO: Check why clang-format fucks this up
    {
        t.write(sendBuf, len)
        } -> std::integral;
};

/**
 * @brief Write a std::span to a writable object
 *
 * @param communicationInterface A writable object.
 * @param data The data to write to the communication interface.
 */
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

/**
 * @brief Write a std::string_view to a writable object
 *
 * @param communicationInterface A writable object.
 * @param message The data to write to the communication interface.
 *
 */
inline auto WriteTo(Writable auto * communicationInterface, std::string_view message)
{
    std::size_t nSentBytes = 0;
    while(nSentBytes < message.size())
    {
        nSentBytes +=
            communicationInterface->write(message.data() + nSentBytes, message.size() - nSentBytes);
    }
}

}
