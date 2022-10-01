#pragma once

#include <cstdint>
#include <span>
#include <vector>

namespace sts1cobcsw::util
{

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
 * @brief Converts two bytes into a uint32_t
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
}