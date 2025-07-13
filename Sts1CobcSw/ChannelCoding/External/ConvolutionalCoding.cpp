// Viterbi Codec.
//
// Author: Min Xu <xukmin@gmail.com>
// Date: 01/30/2015
// Modified by: Tomoya Hagen <tomoya.hagen@spaceteam.at>
// Date: 25/06/2025
// Copyright (C) 2015 Min Xu
// Copyright (C) 2025 Tomoya Hagen
// License: Apache-2.0

#include <Sts1CobcSw/ChannelCoding/External/ConvolutionalCoding.hpp>

#include <etl/utility.h>

#ifdef DISABLE_CHANNEL_CODING
    #include <algorithm>
#endif
#include <cassert>


namespace sts1cobcsw::cc
{
using sts1cobcsw::operator""_b;


ViterbiCodec::ViterbiCodec()
{
    assert(!polynomials.empty());
    for(auto i = 0U; i < polynomials.size(); i++)
    {
        assert(polynomials[i] > 0);
        assert(polynomials[i] < (1 << constraint));
    }
    InitializeOutputs();
    state_ = 0;
    bytes_ = 0;
}

auto ViterbiCodec::NumParityBits() const -> std::size_t
{
    return polynomials.size();
}

auto ViterbiCodec::NextState(unsigned int currentState, unsigned int input) const -> unsigned int
{
    return (currentState >> 1U) | (input << (constraint - 2U));
}

auto ViterbiCodec::Output(unsigned int currentState, unsigned int input) const -> std::uint8_t
{
    return outputs[currentState | (input << (constraint - 1U))];
}

auto ViterbiCodec::ProcessTwoBits(std::uint8_t bit1, std::uint8_t bit2) -> std::uint8_t
{
    auto output1 = Output(state_, bit1);
    state_ = NextState(state_, bit1);
    auto output2 = Output(state_, bit2);
    state_ = NextState(state_, bit2);
    auto output = (output1 << 1) | (output2 & 1);
    assert(output >= 0 && output <= 0b111);
    return static_cast<std::uint8_t>(output);
}

auto ViterbiCodec::Encode(std::span<Byte const> data, [[maybe_unused]] bool flush)
    -> etl::vector<Byte, ViterbiCodec::maxSize>
{
#ifdef DISABLE_CHANNEL_CODING
    auto dst = etl::vector<Byte, ViterbiCodec::maxSize>();
    dst.uninitialized_resize(std::min(data.size(), dst.max_size()));
    std::copy_n(data.begin(), dst.size(), dst.begin());
    return dst;
#else
    auto i = 0U;
    auto dst = etl::vector<Byte, ViterbiCodec::maxSize>();
    for(; i < data.size(); i++)
    {
        auto inputBits = static_cast<std::uint8_t>(data[i]);
        for(auto j = CHAR_BIT - 1; j > 0; j -= 2)  // per iteration 12 bits.
        {
            std::uint8_t bit1 = (inputBits >> static_cast<std::uint8_t>(j)) & 1U;
            std::uint8_t bit2 = (inputBits >> (static_cast<std::uint8_t>(j) - 1U)) & 1U;
            bytes_ = (bytes_ << 3) | ProcessTwoBits(bit1, bit2);
        }

        if(i % 2 == 1)  // when i is odd, the total bits are 24, thus 3 bytes, thus complete to
                        // "push" to output buffer.
        {
            assert(bytes_ <= 0xFF'FFFF);
            auto mask = 0xFF'0000U;
            auto shift = 2U * CHAR_BIT;
            for(auto k = 0; k < 3; ++k)
            {
                dst.push_back(static_cast<Byte>((bytes_ & mask) >> shift));
                mask >>= CHAR_BIT;
                shift -= CHAR_BIT;
            }
            bytes_ = 0;
        }
    }
    if(flush)
    {
        for(int j = constraint - 1; j > 0; j -= 2)
        {
            bytes_ = (bytes_ << 3) | ProcessTwoBits(0, 0);
        }

        if(i % 2 == 1)
        {  // 21 bits to process e.g.: 111101010001111011011
            assert(bytes_ <= 0x1F'FFFF);
            dst.push_back(static_cast<Byte>((bytes_ >> 13) & 0xFF));  // 11110101
            dst.push_back(static_cast<Byte>((bytes_ >> 5) & 0xFF));   // 00011110
            dst.push_back(static_cast<Byte>((bytes_ & 0x1F) << 3));   // 11011 -> 11011000
        }
        else
        {  // 9 bits to process
            assert(bytes_ <= 0x1FF);
            dst.push_back(static_cast<Byte>((bytes_ >> 1) & 0xFF));
            dst.push_back(static_cast<Byte>((bytes_ & 1) << 7));  // 1 -> 10000000
        }
    }
    else
    {
        if(i % 2 == 1)
        {  // 12 bits to process
            assert(bytes_ <= 0xFFF);
            dst.push_back(static_cast<Byte>((bytes_ >> 4) & 0xFF));
            dst.push_back(static_cast<Byte>((bytes_ & 0xF) << 4));
        }
    }
    if(flush)
    {
        state_ = 0;
        bytes_ = 0;
    }
    return dst;
#endif
}

auto ViterbiCodec::InitializeOutputs() -> void
{
    for(auto i = 0U; i < outputs.size(); ++i)
    {
        outputs[i] = 0U;
        for(auto j = 0U; j < NumParityBits(); ++j)
        {
            auto polynomial = polynomials[j];
            auto input = static_cast<std::uint8_t>(i);
            std::uint8_t output = 0U;
            for(auto k = 0U; k < constraint; ++k)
            {
                output ^= (input & 1U) & (polynomial & 1U);
                polynomial >>= 1U;
                input >>= 1U;
            }
            outputs[i] = static_cast<std::uint8_t>((outputs[i] << 1U) | output);
        }
    }
}
}
