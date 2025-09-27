// Viterbi Codec.
//
// Author: Min Xu <xukmin@gmail.com>
// Date: 01/30/2015
// Modified by: Tomoya Hagen <tomoya.hagen@spaceteam.at>
// Modified by: Patrick Kappl <patrick.kappl@spaceteam.at>
// Copyright (C) 2015 Min Xu
// Copyright (C) 2025 Tomoya Hagen
// Copyright (C) 2025 Patrick Kappl
// License: Apache-2.0

#pragma once

#include <Sts1CobcSw/Serial/Byte.hpp>

#include <etl/vector.h>

#include <array>
#include <climits>  // IWYU pragma: keep
#include <cstddef>
#include <cstdint>
#include <span>


namespace sts1cobcsw::cc
{
// This class implements a convolutional Encoder.
class ViterbiCodec
{
public:
    static constexpr auto constraint = 7U;
    static constexpr auto nFlushBits = constraint - 1U;
    // [79, -109], 79 is in hex but I have no idea how the -109 is related to -0b101'1011
    static constexpr auto polynomials = std::to_array<std::int8_t>({0b111'1001, -0b101'1011});
    static constexpr auto asdf = polynomials[0];
    static constexpr auto asdf2 = polynomials[1];
    static constexpr auto nParityBits = polynomials.size();

    static constexpr auto maxUnencodedSize = 255U + 4U;  // >= RS(255,223) + 4 byte sync marker
    // EncodedSize() cannot be used in a constant expression until the class is complete
#if defined(DISABLE_CHANNEL_CODING) || defined(DISABLE_CONVOLUTIONAL_CODING)
    static constexpr auto maxEncodedSize = maxUnencodedSize;
#else
    #ifdef USE_PUNCTURING
    static constexpr auto maxEncodedSize = 390U;
    #else
    static constexpr auto maxEncodedSize = 520U;
    #endif
#endif

    [[nodiscard]] static constexpr auto EncodedSize(std::size_t unencodedSize,
                                                    [[maybe_unused]] bool withFlushBits)
        -> std::size_t;
    [[nodiscard]] static constexpr auto UnencodedSize(std::size_t encodedSize,
                                                      [[maybe_unused]] bool withFlushBits)
        -> std::size_t;

    // Note about Polynomial Descriptor of a Convolutional Encoder / Decoder.
    // A generator polymonial is built as follows: Build a binary number
    // representation by placing a 1 in each spot where a connection line from
    // the shift register feeds into the adder, and a zero elsewhere. There are 2
    // ways to arrange the bits:
    // 1. msb-current
    //    The MSB of the polynomial corresponds to the current input, while the
    //    LSB corresponds to the oldest input that still remains in the shift
    //    register.
    //    This representation is used by MATLAB. See
    //    http://radio.feld.cvut.cz/matlab/toolbox/comm/tutor124.html
    // 2. lsb-current
    //    The LSB of the polynomial corresponds to the current input, while the
    //    MSB corresponds to the oldest input that still remains in the shift
    //    register.
    //    This representation is used by the Spiral Viterbi Decoder Software
    //    Generator. See http://www.spiral.net/software/viterbi.html
    // We use 2.
    ViterbiCodec();
    [[nodiscard]] auto Encode(std::span<Byte const> data, bool flush)
        -> etl::vector<Byte, maxEncodedSize>;


private:
    // The output table.
    // The index is current input bit combined with previous inputs in the shift
    // register. The value is the output parity bits in string format for
    // convenience, e.g. "10". For example, suppose the shift register contains
    // 0b10 (= 2), and the current input is 0b1 (= 1), then the index is 0b110 (=
    // 6).
    static inline auto outputs = std::array<std::uint8_t, 1U << constraint>();

    unsigned int state_ = 0;
    [[maybe_unused]] unsigned int bytes_ = 0;
#ifdef USE_PUNCTURING
    unsigned int nProcessedBytes_ = 0;
#endif

    auto InitializeOutputs() -> void;
    [[nodiscard]] auto NextState(unsigned int currentState, unsigned int input) const
        -> unsigned int;
    [[nodiscard]] auto Output(unsigned int currentState, unsigned int input) const -> std::uint8_t;
    [[nodiscard]] auto ProcessTwoBits(std::uint8_t bit1, std::uint8_t bit2) -> std::uint8_t;
};


constexpr auto ViterbiCodec::EncodedSize(std::size_t unencodedSize,
                                         [[maybe_unused]] bool withFlushBits) -> std::size_t
{
#if defined(DISABLE_CHANNEL_CODING) || defined(DISABLE_CONVOLUTIONAL_CODING)
    return unencodedSize;
#else
    auto flushingBits = withFlushBits ? nFlushBits : 0U;
    #ifdef USE_PUNCTURING
    auto bits = (unencodedSize * CHAR_BIT + flushingBits) * 3 / 2;
    #else
    auto bits = (unencodedSize * CHAR_BIT + flushingBits) * 2;
    #endif
    auto size = (bits + CHAR_BIT - 1) / CHAR_BIT;  // Round up
    return size;
#endif
}


constexpr auto ViterbiCodec::UnencodedSize(std::size_t encodedSize,
                                           [[maybe_unused]] bool withFlushBits) -> std::size_t
{
#if defined(DISABLE_CHANNEL_CODING) || defined(DISABLE_CONVOLUTIONAL_CODING)
    return encodedSize;
#else
    auto flushingBits = withFlushBits ? nFlushBits : 0U;
    #ifdef USE_PUNCTURING
    auto size = (((encodedSize * CHAR_BIT) * 2 / 3) - flushingBits) / CHAR_BIT;
    #else
    auto size = (((encodedSize * CHAR_BIT) / 2) - flushingBits) / CHAR_BIT;
    #endif
    return size;
#endif
}


static_assert(ViterbiCodec::maxEncodedSize
              == ViterbiCodec::EncodedSize(ViterbiCodec::maxUnencodedSize, true));
}
