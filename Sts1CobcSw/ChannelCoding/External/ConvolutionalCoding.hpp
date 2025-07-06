// Viterbi Codec.
//
// Author: Min Xu <xukmin@gmail.com>
// Date: 01/30/2015
// Modified by: Tomoya Hagen <tomoya.hagen@spaceteam.at>
// Date: 25/06/2025
// Copyright (C) 2015 Min Xu
// Copyright (C) 2025 Tomoya Hagen
// License: Apache-2.0

#ifndef VITERBI_H_
#define VITERBI_H_

#include <Sts1CobcSw/Serial/Byte.hpp>

#include <etl/vector.h>

#include <array>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <span>


namespace sts1cobcsw::cc
{
// This class implements both a Viterbi Decoder and a Convolutional Encoder.
class ViterbiCodec
{
    static constexpr auto maxSize = (255 * 3 / 2) + 1;
    static constexpr auto constraint = 7U;
    static constexpr auto nFlushBits = constraint - 1U;
    static constexpr auto polynomials = std::array<std::uint8_t, 2>{0b111'1001, 0b101'1011};
    unsigned int state_;
    unsigned int bytes_;

    // The output table.
    // The index is current input bit combined with previous inputs in the shift
    // register. The value is the output parity bits in string format for
    // convenience, e.g. "10". For example, suppose the shift register contains
    // 0b10 (= 2), and the current input is 0b1 (= 1), then the index is 0b110 (=
    // 6).
    static inline auto outputs = std::array<std::uint8_t, 1U << constraint>();

    static constexpr auto puncturingPattern = std::array{true, true, false, true};

    [[nodiscard]] auto NumParityBits() const -> std::size_t;

    auto InitializeOutputs() -> void;

    [[nodiscard]] auto NextState(unsigned int currentState, unsigned int input) const
        -> unsigned int;

    [[nodiscard]] auto Output(unsigned int currentState, unsigned int input) const -> std::uint8_t;

    auto ProcessTwoBits(std::uint8_t bit1, std::uint8_t bit2) -> std::uint8_t;

public:
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

    [[nodiscard]] static constexpr auto EncodedSize(std::size_t unencodedSize,
                                                    [[maybe_unused]] bool withFlushBits)
        -> std::size_t
    {
#ifdef DISABLE_CHANNEL_CODING
        return unencodedSize;
#else
        auto flushingBits = withFlushBits ? nFlushBits : 0U;
        auto bits = (unencodedSize * CHAR_BIT + flushingBits) * 3 / 2;
        auto size = (bits + CHAR_BIT - 1) / CHAR_BIT;
        return size;
#endif
    }

    [[nodiscard]] static constexpr auto UnencodedSize(std::size_t encodedSize,
                                                      [[maybe_unused]] bool withFlushBits)
        -> std::size_t
    {
#ifdef DISABLE_CHANNEL_CODING
        return encodedSize;
#else
        auto flushingBits = withFlushBits ? nFlushBits : 0U;
        auto size = (((encodedSize * CHAR_BIT) * 2 / 3) - flushingBits) / CHAR_BIT;
        return size % 2 == 0 ? size : size - 1;
#endif
    }

    [[nodiscard]] auto Encode(std::span<Byte const> data, bool flush) -> etl::vector<Byte, maxSize>;

    [[nodiscard]] static auto Constraint() -> int;

    [[nodiscard]] static auto Polynomials() -> std::array<std::uint8_t, 2> const &;
};
}
#endif  // VITERBI_H_
