#pragma once

#include <Sts1CobcSw/Utility/UtilityNames.hpp>

#include <array>
#include <cstdint>
#include <span>

namespace sts1cobcsw::utility
{
//! @brief Implementation of the CRC32/MPEG-2 algorithm.

//! https://en.wikipedia.org/wiki/Cyclic_redundancy_check           -> What is CRC
//! https://reveng.sourceforge.io/crc-catalogue/all.htm             -> Description of MPEG-2
//! implementation https://docs.rs/crc/3.0.0/src/crc/crc32.rs.html  -> Rust implementation (EDU)
//! https://gist.github.com/Miliox/b86b60b9755faf3bd7cf             -> C++ implementation
//! of MPEG-2 https://crccalc.com/                                  -> To check the implementation
//!
//! @param data The data over which the checksum is calculated
//! @returns The 32 bit checksum
auto Crc32(std::span<uint8_t> data) -> uint32_t;
}
