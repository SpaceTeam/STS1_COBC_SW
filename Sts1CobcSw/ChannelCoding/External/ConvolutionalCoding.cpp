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

#include <Sts1CobcSw/ChannelCoding/External/ConvolutionalCoding.hpp>

#include <etl/utility.h>

#if defined(DISABLE_CHANNEL_CODING) || defined(DISABLE_CONVOLUTIONAL_CODING)
    #include <algorithm>
#endif
#include <cassert>
#include <cstdlib>


namespace sts1cobcsw::cc
{
using sts1cobcsw::operator""_b;


ViterbiCodec::ViterbiCodec()
{
    assert(!polynomials.empty());
    for(auto i = 0U; i < polynomials.size(); i++)
    {
        assert(std::abs(polynomials[i]) < (1 << constraint));
    }
    InitializeOutputs();
}


auto ViterbiCodec::Encode(std::span<Byte const> data, [[maybe_unused]] bool flush)
    -> etl::vector<Byte, ViterbiCodec::maxEncodedSize>
{
    assert(data.size() <= ViterbiCodec::maxUnencodedSize);

    auto dst = etl::vector<Byte, ViterbiCodec::maxEncodedSize>();
#if defined(DISABLE_CHANNEL_CODING) || defined(DISABLE_CONVOLUTIONAL_CODING)
    dst.uninitialized_resize(std::min(data.size(), dst.max_size()));
    std::copy_n(data.begin(), dst.size(), dst.begin());
#elif defined(USE_PUNCTURING)
    for(auto i = 0U; i < data.size(); ++i)
    {
        auto inputBits = static_cast<std::uint8_t>(data[i]);
        // Turn 8 input bits into 12 encoded bits
        for(auto j = CHAR_BIT - 1; j > 0; j -= 2)
        {
            std::uint8_t bit1 = (inputBits >> static_cast<std::uint8_t>(j)) & 1U;
            std::uint8_t bit2 = (inputBits >> (static_cast<std::uint8_t>(j) - 1U)) & 1U;
            bytes_ = (bytes_ << 3) | ProcessTwoBits(bit1, bit2);
        }
        if((i + nProcessedBytes_) % 2 == 0)
        {
            // 12 bits -> push back 1 byte and keep the remaining 4 bits for the next iteration
            assert(bytes_ <= 0xFFF);
            dst.push_back(static_cast<Byte>((bytes_ >> 4) & 0xFF));
            bytes_ &= 0xF;
        }
        else
        {
            // 12 bits + 4 bits from the last iteration -> push back 2 bytes
            assert(bytes_ <= 0xFFFF);
            dst.push_back(static_cast<Byte>((bytes_ >> 8) & 0xFF));
            dst.push_back(static_cast<Byte>(bytes_ & 0xFF));
            bytes_ = 0;
        }
    }
    nProcessedBytes_ += data.size();
    if(flush)
    {
        for(int j = constraint - 1; j > 0; j -= 2)
        {
            bytes_ = (bytes_ << 3) | ProcessTwoBits(0, 0);
        }
        if(nProcessedBytes_ % 2 == 1)
        {
            // 13 bits remaining -> pad with 3 zeros from the right to make it 2 full bytes
            assert(bytes_ <= 0x1FFF);
            bytes_ <<= 3;
        }
        else
        {
            // 9 bits remaining -> pad with 7 zeros from the right to make it 2 full bytes
            assert(bytes_ <= 0x1FF);
            bytes_ <<= 7;
        }
        dst.push_back(static_cast<Byte>((bytes_ >> 8) & 0xFF));
        dst.push_back(static_cast<Byte>(bytes_ & 0xFF));
        state_ = 0;
        bytes_ = 0;
        nProcessedBytes_ = 0;
    }
    // else
    // {
    //     if(nProcessedBytes_ % 2 == 1)
    //     {
    //         assert(bytes_ <= 0xFFF);  // 12 bits -> we can output 1 more full byte
    //         dst.push_back(static_cast<Byte>((bytes_ >> 4) & 0xFF));
    //         bytes_ &= 0xF;  // Keep the last 4 bits for the next call
    //         ++nProcessedBytes_;
    //     }
    // }
#else
    for(auto i = 0U; i < data.size(); i++)
    {
        auto inputBits = static_cast<std::uint8_t>(data[i]);
        for(auto j = CHAR_BIT - 1; j > 0; j -= 2)
        {
            std::uint8_t bit1 = (inputBits >> static_cast<std::uint8_t>(j)) & 1U;
            std::uint8_t bit2 = (inputBits >> (static_cast<std::uint8_t>(j) - 1U)) & 1U;
            bytes_ = (bytes_ << 4) | ProcessTwoBits(bit1, bit2);
        }

        assert(bytes_ <= 0xFFFF);
        auto mask = 0xFF00U;

        dst.push_back(static_cast<Byte>((bytes_ & mask) >> CHAR_BIT));
        dst.push_back(static_cast<Byte>(bytes_ & (mask >> CHAR_BIT)));
        bytes_ = 0;
    }
    if(flush)
    {
        for(int j = constraint - 1; j > 0; j -= 2)
        {
            bytes_ = (bytes_ << 4) | ProcessTwoBits(0, 0);
        }

        assert(bytes_ <= 0xFFF);
        dst.push_back(static_cast<Byte>((bytes_ >> 4) & 0xFF));
        dst.push_back(static_cast<Byte>((bytes_ & 0xF) << 4));
        state_ = 0;
        bytes_ = 0;
    }
#endif
    return dst;
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
#ifdef USE_PUNCTURING
    auto output = (output1 << 1) | (output2 & 1);
    assert(output <= 0b111);
#else
    auto output = (output1 << 2) | output2;
    assert(output <= 0b1111);
#endif
    return static_cast<std::uint8_t>(output);
}


auto ViterbiCodec::InitializeOutputs() -> void
{
    for(auto i = 0U; i < outputs.size(); ++i)
    {
        outputs[i] = 0U;
        for(auto j = 0U; j < nParityBits; ++j)
        {
            auto polynomial = static_cast<std::uint8_t>(std::abs(polynomials[j]));
            auto input = static_cast<std::uint8_t>(i);
            std::uint8_t output = 0U;
            for(auto k = 0U; k < constraint; ++k)
            {
                output ^= static_cast<std::uint8_t>((polynomials[j] < 0)
                                                    ^ ((input & 1U) & (polynomial & 1U)));
                polynomial >>= 1U;
                input >>= 1U;
            }
            outputs[i] = static_cast<std::uint8_t>((outputs[i] << 1U) | output);
        }
    }
}
}
