#include <Tests/CatchRodos/TestMacros.hpp>

#include <Sts1CobcSw/ChannelCoding/ChannelCoding.hpp>
#include <Sts1CobcSw/ChannelCoding/External/ConvolutionalCoding.hpp>
#include <Sts1CobcSw/ChannelCoding/ReedSolomon.hpp>
#include <Sts1CobcSw/ChannelCoding/Scrambler.hpp>
#include <Sts1CobcSw/Outcome/Outcome.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>

#include <etl/vector.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <random>
#include <span>


namespace rs = sts1cobcsw::rs;
namespace cc = sts1cobcsw::cc;
using sts1cobcsw::Byte;
using sts1cobcsw::operator""_b;


namespace
{
template<std::size_t size>
constexpr auto GenerateSequentialBytes(Byte startValue = 0_b) -> std::array<Byte, size>
{
    auto result = std::array<Byte, size>{};
    for(auto i = 0U; i < size; ++i)
    {
        result[i] = static_cast<Byte>(static_cast<std::uint8_t>(startValue) + i);
    }
    return result;
}
}


TEST_CASE("Convolutional coding")
{
    // EncodedSize() and UnencodedSize()
    {
        static_assert(cc::ViterbiCodec::EncodedSize(100, false) == 150);
        static_assert(cc::ViterbiCodec::EncodedSize(100, true) == 152);
        static_assert(cc::ViterbiCodec::EncodedSize(101, false) == 152);
        static_assert(cc::ViterbiCodec::EncodedSize(101, true) == 153);
        static_assert(cc::ViterbiCodec::EncodedSize(102, false) == 153);
        static_assert(cc::ViterbiCodec::EncodedSize(102, true) == 155);
        static_assert(cc::ViterbiCodec::EncodedSize(103, false) == 155);
        static_assert(cc::ViterbiCodec::EncodedSize(103, true) == 156);

        static_assert(cc::ViterbiCodec::UnencodedSize(152, false) == 100);
        static_assert(cc::ViterbiCodec::UnencodedSize(152, true) == 100);
        static_assert(cc::ViterbiCodec::UnencodedSize(153, false) == 102);
        static_assert(cc::ViterbiCodec::UnencodedSize(155, false) == 102);
        static_assert(cc::ViterbiCodec::UnencodedSize(156, false) == 104);
        static_assert(cc::ViterbiCodec::UnencodedSize(275, false) == 182);
        static_assert(cc::ViterbiCodec::UnencodedSize(275, true) == 182);
        static_assert(cc::ViterbiCodec::UnencodedSize(276, true) == 182);
        static_assert(cc::ViterbiCodec::UnencodedSize(383, false) == 254);
    }

    // Encode() returns the correct amount of data
    {
        auto cc = cc::ViterbiCodec();
        REQUIRE(cc.Encode(std::array<Byte, 10>{}, /*flush=*/false).size() == 15U);
        REQUIRE(cc.Encode(std::array<Byte, 11>{}, /*flush=*/false).size() == 17U);
        REQUIRE(cc.Encode(std::array<Byte, 10>{}, /*flush=*/true).size() == 17U);
        REQUIRE(cc.Encode(std::array<Byte, 11>{}, /*flush=*/true).size() == 18U);
    }

    static constexpr auto inputSize = 255U;
    static constexpr auto message = GenerateSequentialBytes<inputSize>();
    // We have to use std::to_array here because deducing the type of the array with 384
    // elements exceeds the fold expression limit of 256
    static constexpr auto correctlyEncodedMessage = std::to_array<Byte>(
        {0_b,   0_b,   1_b,   185_b, 238_b, 119_b, 94_b,  92_b,  204_b, 117_b, 50_b,  186_b, 145_b,
         243_b, 182_b, 130_b, 157_b, 192_b, 101_b, 47_b,  123_b, 78_b,  65_b,  13_b,  172_b, 102_b,
         111_b, 223_b, 8_b,   25_b,  56_b,  186_b, 162_b, 19_b,  212_b, 212_b, 247_b, 21_b,  216_b,
         228_b, 123_b, 174_b, 3_b,   201_b, 21_b,  40_b,  167_b, 99_b,  209_b, 29_b,  184_b, 98_b,
         115_b, 206_b, 133_b, 193_b, 117_b, 174_b, 175_b, 3_b,   74_b,  110_b, 15_b,  89_b,  0_b,
         121_b, 190_b, 178_b, 194_b, 149_b, 220_b, 180_b, 119_b, 251_b, 214_b, 4_b,   149_b, 160_b,
         227_b, 39_b,  27_b,  200_b, 73_b,  109_b, 44_b,  136_b, 97_b,  63_b,  230_b, 23_b,  216_b,
         84_b,  172_b, 243_b, 58_b,  218_b, 61_b,  179_b, 114_b, 142_b, 221_b, 4_b,   105_b, 111_b,
         191_b, 66_b,  1_b,   201_b, 166_b, 192_b, 197_b, 181_b, 174_b, 179_b, 82_b,  28_b,  8_b,
         121_b, 114_b, 126_b, 155_b, 85_b,  28_b,  232_b, 59_b,  106_b, 15_b,  137_b, 209_b, 36_b,
         231_b, 167_b, 192_b, 38_b,  171_b, 211_b, 72_b,  221_b, 52_b,  250_b, 102_b, 31_b,  148_b,
         16_b,  230_b, 46_b,  203_b, 85_b,  64_b,  189_b, 178_b, 242_b, 6_b,   153_b, 156_b, 112_b,
         125_b, 93_b,  124_b, 110_b, 51_b,  10_b,  137_b, 129_b, 177_b, 162_b, 239_b, 199_b, 64_b,
         200_b, 165_b, 51_b,  166_b, 211_b, 212_b, 20_b,  104_b, 255_b, 122_b, 30_b,  27_b,  187_b,
         18_b,  8_b,   213_b, 100_b, 239_b, 103_b, 223_b, 196_b, 9_b,   169_b, 214_b, 109_b, 207_b,
         101_b, 3_b,   185_b, 130_b, 177_b, 2_b,   169_b, 223_b, 116_b, 77_b,  30_b,  120_b, 94_b,
         112_b, 14_b,  185_b, 194_b, 181_b, 146_b, 172_b, 195_b, 112_b, 139_b, 161_b, 3_b,   229_b,
         215_b, 228_b, 87_b,  108_b, 207_b, 57_b,  26_b,  43_b,  248_b, 22_b,  56_b,  150_b, 96_b,
         223_b, 36_b,  219_b, 244_b, 74_b,  173_b, 13_b,  240_b, 118_b, 190_b, 158_b, 0_b,   89_b,
         44_b,  187_b, 114_b, 66_b,  205_b, 150_b, 131_b, 193_b, 133_b, 237_b, 183_b, 98_b,  95_b,
         12_b,  73_b,  49_b,  122_b, 171_b, 22_b,  24_b,  216_b, 120_b, 110_b, 63_b,  202_b, 213_b,
         20_b,  164_b, 163_b, 240_b, 101_b, 175_b, 227_b, 11_b,  217_b, 4_b,   185_b, 98_b,  47_b,
         215_b, 20_b,  225_b, 94_b,  188_b, 82_b,  48_b,  202_b, 181_b, 130_b, 113_b, 158_b, 236_b,
         7_b,   122_b, 45_b,  11_b,  105_b, 67_b,  125_b, 142_b, 241_b, 198_b, 165_b, 159_b, 176_b,
         71_b,  184_b, 210_b, 52_b,  214_b, 164_b, 211_b, 100_b, 31_b,  248_b, 10_b,  105_b, 28_b,
         203_b, 101_b, 15_b,  165_b, 19_b,  232_b, 23_b,  168_b, 195_b, 121_b, 222_b, 58_b,  195_b,
         5_b,   137_b, 173_b, 115_b, 110_b, 31_b,  200_b, 69_b,  113_b, 190_b, 161_b, 176_b, 178_b,
         178_b, 222_b, 196_b, 85_b,  108_b, 127_b, 126_b, 2_b,   9_b,   156_b, 37_b,  107_b, 239_b,
         75_b,  29_b,  8_b,   249_b, 166_b, 35_b,  151_b, 208_b, 199_b, 86_b,  220_b, 212_b, 56_b,
         170_b, 51_b,  138_b, 17_b,  24_b,  235_b, 48_b});
    auto cc = cc::ViterbiCodec();

    // Endode() returns the correct data
    {
        auto encodedMessage = cc.Encode(message, /*flush=*/true);
        REQUIRE(encodedMessage.size() == correctlyEncodedMessage.size());
        // There is no operator==() for etl::vector and std:array. Since CHECK(array == array) gives
        // a better error message when the test fails than looping over all elements, we copy the
        // elements to a new array and compare that.
        auto encodedMessageArray = decltype(correctlyEncodedMessage){};
        std::ranges::copy(encodedMessage, encodedMessageArray.begin());
        CHECK(encodedMessageArray == correctlyEncodedMessage);
    }

    // Encoding the message in randomly-sized chunks gives the same result
    {
        auto rd = std::random_device{};
        auto gen = std::mt19937(rd());
        auto distribution = std::uniform_int_distribution(25, 98);
        auto freeSpace = static_cast<unsigned>(distribution(gen));
        auto finalChunkSize = cc::ViterbiCodec::UnencodedSize(freeSpace, /*withFlushBits*/ true);
        auto dataIndex = 0U;
        auto buffer = etl::vector<Byte, correctlyEncodedMessage.size()>{};
        while(dataIndex + finalChunkSize < message.size())
        {
            auto chunkSize = cc::ViterbiCodec::UnencodedSize(freeSpace, /*withFlushBits*/ false);
            auto encodedChunk =
                cc.Encode(std::span(message).subspan(dataIndex, chunkSize), /*flush=*/false);
            buffer.insert(buffer.end(), encodedChunk.begin(), encodedChunk.end());
            dataIndex += chunkSize;
            freeSpace = static_cast<unsigned>(distribution(gen));
            finalChunkSize = cc::ViterbiCodec::UnencodedSize(freeSpace, /*withFlushBits*/ true);
        }
        auto encodedChunk = cc.Encode(std::span(message).subspan(dataIndex), /*flush=*/true);
        buffer.insert(buffer.end(), encodedChunk.begin(), encodedChunk.end());
        REQUIRE(buffer.size() == correctlyEncodedMessage.size());
        auto encodedMessage = decltype(correctlyEncodedMessage){};
        std::ranges::copy(buffer, encodedMessage.begin());
        CHECK(encodedMessage == correctlyEncodedMessage);
    }
}


TEST_CASE("Reed-Solomon")
{
    auto block = std::array<Byte, rs::blockLength>{};
    auto message = std::array<Byte, rs::messageLength>{};
    auto paritySymbols = std::array<Byte, rs::nParitySymbols>{};
    static constexpr auto originalMessage = GenerateSequentialBytes<rs::messageLength>();

    // Encode() computes the correct parity symbols
    {
        std::ranges::copy(originalMessage, block.begin());
        rs::Encode(std::span(block).first<rs::messageLength>(),
                   std::span(block).last<rs::nParitySymbols>());
        static constexpr auto correctParitySymbols = std::array{
            0x4f_b, 0xfb_b, 0x92_b, 0xdd_b, 0x55_b, 0x7e_b, 0xc6_b, 0x7f_b, 0x27_b, 0xfb_b, 0x89_b,
            0x82_b, 0xcf_b, 0x58_b, 0xf8_b, 0xfd_b, 0x02_b, 0x8a_b, 0xd1_b, 0x17_b, 0xfc_b, 0xef_b,
            0x6b_b, 0x27_b, 0x93_b, 0xd0_b, 0x41_b, 0x88_b, 0x26_b, 0x57_b, 0x86_b, 0x51_b};
        std::ranges::copy(std::span(block).last<rs::nParitySymbols>(), paritySymbols.begin());
        CHECK(paritySymbols == correctParitySymbols);
    }

    // Up to nParitySymbols / 2 errors can be corrected
    {
        static constexpr auto nErrors = sts1cobcsw::nParitySymbols / 2U;

        std::ranges::copy(originalMessage, block.begin());
        rs::Encode(std::span(block).first<rs::messageLength>(),
                   std::span(block).last<rs::nParitySymbols>());
        for(auto i = 0U; i < nErrors; ++i)
        {
            block[i] ^= 0xFF_b;
        }
        auto decodeResult = rs::Decode(block);
        CHECK(decodeResult.has_value());
        CHECK(decodeResult.value() == static_cast<int>(nErrors));
        std::copy_n(block.begin(), message.size(), message.begin());
        CHECK(message == originalMessage);

        for(auto i = 0U; i < nErrors / 2; ++i)
        {
            block[5 * i + 2] ^= 0xFF_b;
        }
        decodeResult = rs::Decode(block);
        CHECK(decodeResult.has_value());
        CHECK(decodeResult.value() == static_cast<int>(nErrors / 2));
        std::copy_n(block.begin(), message.size(), message.begin());
        CHECK(message == originalMessage);

        for(auto i = 0U; i < nErrors + 1U; ++i)
        {
            block[i] ^= 0xFF_b;
        }
        decodeResult = rs::Decode(block);
        CHECK(decodeResult.has_error());
        CHECK(decodeResult.error() == sts1cobcsw::ErrorCode::errorCorrectionFailed);
        std::copy_n(block.begin(), message.size(), message.begin());
        CHECK(message != originalMessage);
    }
}


TEST_CASE("Scrambler")
{
    static constexpr auto originalData = GenerateSequentialBytes<rs::blockLength>();

    // Unscramble() is the inverse of Scramble()
    {
        auto data = originalData;
        sts1cobcsw::tc::Scramble(data);
        sts1cobcsw::tc::Unscramble(data);
        CHECK(data == originalData);

        sts1cobcsw::tm::Scramble(data);
        sts1cobcsw::tm::Unscramble(data);
        CHECK(data == originalData);
    }

    // Regression tests
    {
        auto data = originalData;
        sts1cobcsw::tc::Unscramble(data);
        static constexpr auto correctlyUnscrambledData = std::array{
            0xff_b, 0x38_b, 0x9c_b, 0x59_b, 0x6c_b, 0xec_b, 0x00_b, 0xf2_b, 0x64_b, 0x80_b, 0x25_b,
            0xaa_b, 0x3d_b, 0x53_b, 0x06_b, 0xcf_b, 0x42_b, 0xb9_b, 0xa9_b, 0xbd_b, 0x5a_b, 0xd7_b,
            0xd1_b, 0xfa_b, 0x7e_b, 0xc5_b, 0x22_b, 0xcf_b, 0xe4_b, 0x9b_b, 0x4e_b, 0x22_b, 0xde_b,
            0x52_b, 0x1e_b, 0x97_b, 0xf5_b, 0xf7_b, 0x2b_b, 0xcd_b, 0xf1_b, 0x3b_b, 0x75_b, 0x69_b,
            0x4e_b, 0x91_b, 0x3f_b, 0xaf_b, 0x95_b, 0x60_b, 0x45_b, 0x6f_b, 0xa9_b, 0xb0_b, 0xb9_b,
            0xed_b, 0xf5_b, 0x81_b, 0x4b_b, 0x92_b, 0xcd_b, 0x31_b, 0x9e_b, 0x44_b, 0xbc_b, 0xa7_b,
            0x3b_b, 0x2a_b, 0xe7_b, 0xe1_b, 0x5d_b, 0x92_b, 0xfa_b, 0x6d_b, 0xf4_b, 0xcf_b, 0x89_b,
            0x35_b, 0x6d_b, 0x4e_b, 0x1a_b, 0xf3_b, 0xbc_b, 0xea_b, 0x6f_b, 0x5e_b, 0x49_b, 0xe2_b,
            0xc3_b, 0x29_b, 0xb9_b, 0x08_b, 0xbe_b, 0x44_b, 0x1e_b, 0xa8_b, 0x99_b, 0xad_b, 0x90_b,
            0xb0_b, 0x23_b, 0x2d_b, 0x51_b, 0xcc_b, 0x0c_b, 0x20_b, 0x17_b, 0x62_b, 0xe6_b, 0x9d_b,
            0x28_b, 0x6d_b, 0xe5_b, 0x34_b, 0xaf_b, 0x01_b, 0x02_b, 0x63_b, 0x49_b, 0x1c_b, 0x4e_b,
            0x98_b, 0xbc_b, 0xdc_b, 0xb8_b, 0x4f_b, 0xff_b, 0x90_b, 0x73_b, 0x18_b, 0x67_b, 0x25_b,
            0x0a_b, 0x15_b, 0xe9_b, 0xd1_b, 0x40_b, 0x1b_b, 0x70_b, 0x98_b, 0x99_b, 0x6d_b, 0x02_b,
            0x8a_b, 0xba_b, 0x1a_b, 0x28_b, 0x77_b, 0x78_b, 0xb9_b, 0xe8_b, 0x41_b, 0xf5_b, 0x5a_b,
            0x17_b, 0xd4_b, 0x14_b, 0xf8_b, 0x9d_b, 0x40_b, 0x47_b, 0x92_b, 0x69_b, 0xee_b, 0xb9_b,
            0x85_b, 0x78_b, 0x0a_b, 0x39_b, 0x8c_b, 0x5e_b, 0x8d_b, 0x87_b, 0x6c_b, 0xb6_b, 0xa5_b,
            0xe5_b, 0xa6_b, 0xc7_b, 0x7a_b, 0x6c_b, 0xed_b, 0x4b_b, 0x1b_b, 0x63_b, 0x3e_b, 0xa0_b,
            0x24_b, 0xac_b, 0x77_b, 0xb9_b, 0x00_b, 0x0e_b, 0xa6_b, 0x54_b, 0x59_b, 0xfe_b, 0x84_b,
            0x7b_b, 0x9c_b, 0xea_b, 0x82_b, 0x22_b, 0x87_b, 0x9b_b, 0x4f_b, 0xfe_b, 0xdb_b, 0x7a_b,
            0xff_b, 0x39_b, 0x40_b, 0x64_b, 0x64_b, 0x2d_b, 0x8e_b, 0x6f_b, 0xd7_b, 0xef_b, 0xe5_b,
            0xfd_b, 0x49_b, 0xd1_b, 0xa0_b, 0x7c_b, 0x2e_b, 0xcf_b, 0xd7_b, 0x90_b, 0x66_b, 0x9c_b,
            0x51_b, 0xac_b, 0x7e_b, 0x3a_b, 0x73_b, 0x43_b, 0xe9_b, 0x8e_b, 0xc6_b, 0xa4_b, 0xac_b,
            0x25_b, 0xd4_b, 0x95_b, 0x96_b, 0x00_b, 0x44_b, 0x96_b, 0xe5_b, 0x90_b, 0x87_b, 0xbf_b,
            0xd5_b, 0xe0_b};
        CHECK(data == correctlyUnscrambledData);

        data = originalData;
        sts1cobcsw::tm::Scramble(data);
        static constexpr auto correctlyScrambledData = std::array{
            0xff_b, 0x49_b, 0x0c_b, 0xc3_b, 0x9e_b, 0x08_b, 0x76_b, 0xbb_b, 0x86_b, 0x25_b, 0x99_b,
            0xa6_b, 0xab_b, 0xba_b, 0x48_b, 0xc1_b, 0x4a_b, 0x86_b, 0x6f_b, 0xdf_b, 0x26_b, 0xb7_b,
            0xa9_b, 0x29_b, 0x12_b, 0x09_b, 0xeb_b, 0x93_b, 0x88_b, 0xd0_b, 0xf4_b, 0xae_b, 0xde_b,
            0xb1_b, 0x3f_b, 0xa2_b, 0x10_b, 0x3f_b, 0xc7_b, 0x5e_b, 0x34_b, 0x70_b, 0x0d_b, 0x70_b,
            0x63_b, 0x43_b, 0xa3_b, 0xb3_b, 0x85_b, 0x1f_b, 0xc9_b, 0xab_b, 0x51_b, 0x70_b, 0x48_b,
            0x4b_b, 0x2c_b, 0x18_b, 0xd9_b, 0x2a_b, 0x15_b, 0xa6_b, 0xeb_b, 0x5c_b, 0xbd_b, 0x61_b,
            0x79_b, 0x41_b, 0x2c_b, 0x70_b, 0x84_b, 0xb5_b, 0x70_b, 0xfb_b, 0x04_b, 0xfd_b, 0xd2_b,
            0x90_b, 0x55_b, 0x76_b, 0x3a_b, 0x0c_b, 0xa5_b, 0x63_b, 0x9e_b, 0xdf_b, 0xaa_b, 0xaf_b,
            0x70_b, 0x1a_b, 0x9c_b, 0x79_b, 0x0f_b, 0x6a_b, 0xf4_b, 0x98_b, 0x9a_b, 0x21_b, 0x14_b,
            0x67_b, 0xb4_b, 0x0e_b, 0xe3_b, 0x83_b, 0x19_b, 0x0d_b, 0xf7_b, 0x06_b, 0x51_b, 0xd7_b,
            0x58_b, 0x1d_b, 0xa4_b, 0xca_b, 0x9c_b, 0x12_b, 0xe1_b, 0x60_b, 0x8f_b, 0x87_b, 0x28_b,
            0xfe_b, 0xf6_b, 0x3f_b, 0xda_b, 0x12_b, 0x2b_b, 0xf0_b, 0x74_b, 0x01_b, 0x6e_b, 0x8a_b,
            0x24_b, 0x52_b, 0x8d_b, 0x4f_b, 0x6a_b, 0x40_b, 0xb0_b, 0x51_b, 0xf7_b, 0xf9_b, 0xe2_b,
            0x6a_b, 0x39_b, 0xe6_b, 0x4e_b, 0x50_b, 0xbe_b, 0xbe_b, 0x65_b, 0x77_b, 0x39_b, 0x96_b,
            0x82_b, 0x12_b, 0xd0_b, 0x43_b, 0x35_b, 0x80_b, 0x49_b, 0xa0_b, 0x7a_b, 0xb0_b, 0xe5_b,
            0x0b_b, 0xb1_b, 0x36_b, 0x6d_b, 0x3b_b, 0xdf_b, 0x1f_b, 0x5a_b, 0x45_b, 0x77_b, 0x64_b,
            0xe2_b, 0x5e_b, 0x0b_b, 0x35_b, 0xe0_b, 0xe2_b, 0x51_b, 0x76_b, 0xfa_b, 0xa7_b, 0x8b_b,
            0xa9_b, 0x25_b, 0x00_b, 0xe8_b, 0x80_b, 0x12_b, 0xc2_b, 0x72_b, 0xe5_b, 0x47_b, 0x99_b,
            0xe9_b, 0xe4_b, 0x43_b, 0xed_b, 0x21_b, 0xa2_b, 0x21_b, 0x1c_b, 0x7d_b, 0x59_b, 0x75_b,
            0x0e_b, 0xa1_b, 0xdf_b, 0x7c_b, 0x7a_b, 0x19_b, 0x55_b, 0x5c_b, 0xe5_b, 0xb8_b, 0xfe_b,
            0xef_b, 0xa7_b, 0x72_b, 0xa0_b, 0x44_b, 0xe6_b, 0x82_b, 0xae_b, 0xe2_b, 0x5d_b, 0xb8_b,
            0xa0_b, 0xfe_b, 0xa0_b, 0x3c_b, 0x38_b, 0x37_b, 0x4e_b, 0x89_b, 0xc2_b, 0xbb_b, 0x4f_b,
            0x14_b, 0xea_b, 0xa5_b, 0xaa_b, 0x69_b, 0xf2_b, 0xf0_b, 0x81_b, 0x3e_b, 0xb1_b, 0x9a_b,
            0x08_b, 0xa6_b};
        CHECK(data == correctlyScrambledData);
    }
}


TEST_CASE("Channel coding")
{
    auto block = std::array<Byte, rs::blockLength>{};
    auto message = std::array<Byte, rs::messageLength>{};
    static constexpr auto originalMessage = GenerateSequentialBytes<rs::messageLength>();

    // Decoding is the inverse of encoding
    {
        std::ranges::copy(originalMessage, block.begin());
        sts1cobcsw::tc::Encode(block);
        auto decodeResult = sts1cobcsw::tc::Decode(block);
        CHECK(decodeResult.has_value());
        CHECK(decodeResult.value() == 0);
        std::copy_n(block.begin(), message.size(), message.begin());
        CHECK(message == originalMessage);

        std::ranges::copy(originalMessage, block.begin());
        sts1cobcsw::tm::Encode(block);
        decodeResult = sts1cobcsw::tm::Decode(block);
        CHECK(decodeResult.has_value());
        CHECK(decodeResult.value() == 0);
        std::copy_n(block.begin(), message.size(), message.begin());
        CHECK(message == originalMessage);
    }

    // Up to nParitySymbols / 2 errors can be corrected
    {
        static constexpr auto nErrors = sts1cobcsw::nParitySymbols / 2U;

        std::ranges::copy(originalMessage, block.begin());
        sts1cobcsw::tm::Encode(block);
        for(auto i = 0U; i < nErrors; ++i)
        {
            block[i] ^= 0xFF_b;
        }
        auto decodeResult = sts1cobcsw::tm::Decode(block);
        CHECK(decodeResult.has_value());
        CHECK(decodeResult.value() == static_cast<int>(nErrors));
        std::copy_n(block.begin(), message.size(), message.begin());
        CHECK(message == originalMessage);

        std::ranges::copy(originalMessage, block.begin());
        sts1cobcsw::tc::Encode(block);
        for(auto i = 0U; i < nErrors; ++i)
        {
            block[i] ^= 0xFF_b;
        }
        decodeResult = sts1cobcsw::tc::Decode(block);
        CHECK(decodeResult.has_value());
        CHECK(decodeResult.value() == static_cast<int>(nErrors));
        std::copy_n(block.begin(), message.size(), message.begin());
        CHECK(message == originalMessage);
    }

    // Regression test
    {
        std::ranges::copy(originalMessage, block.begin());
        sts1cobcsw::tm::Encode(block);
        auto correctlyEncodedBlock = std::array{
            0xff_b, 0x49_b, 0x0c_b, 0xc3_b, 0x9e_b, 0x08_b, 0x76_b, 0xbb_b, 0x86_b, 0x25_b, 0x99_b,
            0xa6_b, 0xab_b, 0xba_b, 0x48_b, 0xc1_b, 0x4a_b, 0x86_b, 0x6f_b, 0xdf_b, 0x26_b, 0xb7_b,
            0xa9_b, 0x29_b, 0x12_b, 0x09_b, 0xeb_b, 0x93_b, 0x88_b, 0xd0_b, 0xf4_b, 0xae_b, 0xde_b,
            0xb1_b, 0x3f_b, 0xa2_b, 0x10_b, 0x3f_b, 0xc7_b, 0x5e_b, 0x34_b, 0x70_b, 0x0d_b, 0x70_b,
            0x63_b, 0x43_b, 0xa3_b, 0xb3_b, 0x85_b, 0x1f_b, 0xc9_b, 0xab_b, 0x51_b, 0x70_b, 0x48_b,
            0x4b_b, 0x2c_b, 0x18_b, 0xd9_b, 0x2a_b, 0x15_b, 0xa6_b, 0xeb_b, 0x5c_b, 0xbd_b, 0x61_b,
            0x79_b, 0x41_b, 0x2c_b, 0x70_b, 0x84_b, 0xb5_b, 0x70_b, 0xfb_b, 0x04_b, 0xfd_b, 0xd2_b,
            0x90_b, 0x55_b, 0x76_b, 0x3a_b, 0x0c_b, 0xa5_b, 0x63_b, 0x9e_b, 0xdf_b, 0xaa_b, 0xaf_b,
            0x70_b, 0x1a_b, 0x9c_b, 0x79_b, 0x0f_b, 0x6a_b, 0xf4_b, 0x98_b, 0x9a_b, 0x21_b, 0x14_b,
            0x67_b, 0xb4_b, 0x0e_b, 0xe3_b, 0x83_b, 0x19_b, 0x0d_b, 0xf7_b, 0x06_b, 0x51_b, 0xd7_b,
            0x58_b, 0x1d_b, 0xa4_b, 0xca_b, 0x9c_b, 0x12_b, 0xe1_b, 0x60_b, 0x8f_b, 0x87_b, 0x28_b,
            0xfe_b, 0xf6_b, 0x3f_b, 0xda_b, 0x12_b, 0x2b_b, 0xf0_b, 0x74_b, 0x01_b, 0x6e_b, 0x8a_b,
            0x24_b, 0x52_b, 0x8d_b, 0x4f_b, 0x6a_b, 0x40_b, 0xb0_b, 0x51_b, 0xf7_b, 0xf9_b, 0xe2_b,
            0x6a_b, 0x39_b, 0xe6_b, 0x4e_b, 0x50_b, 0xbe_b, 0xbe_b, 0x65_b, 0x77_b, 0x39_b, 0x96_b,
            0x82_b, 0x12_b, 0xd0_b, 0x43_b, 0x35_b, 0x80_b, 0x49_b, 0xa0_b, 0x7a_b, 0xb0_b, 0xe5_b,
            0x0b_b, 0xb1_b, 0x36_b, 0x6d_b, 0x3b_b, 0xdf_b, 0x1f_b, 0x5a_b, 0x45_b, 0x77_b, 0x64_b,
            0xe2_b, 0x5e_b, 0x0b_b, 0x35_b, 0xe0_b, 0xe2_b, 0x51_b, 0x76_b, 0xfa_b, 0xa7_b, 0x8b_b,
            0xa9_b, 0x25_b, 0x00_b, 0xe8_b, 0x80_b, 0x12_b, 0xc2_b, 0x72_b, 0xe5_b, 0x47_b, 0x99_b,
            0xe9_b, 0xe4_b, 0x43_b, 0xed_b, 0x21_b, 0xa2_b, 0x21_b, 0x1c_b, 0x7d_b, 0x59_b, 0x75_b,
            0x0e_b, 0xa1_b, 0xdf_b, 0x7c_b, 0x7a_b, 0x19_b, 0x55_b, 0x5c_b, 0xe5_b, 0xb8_b, 0xfe_b,
            0xef_b, 0xa7_b, 0x72_b, 0x30_b, 0x5f_b, 0x95_b, 0xbd_b, 0x18_b, 0x78_b, 0x7e_b, 0x21_b,
            0x60_b, 0xed_b, 0xc0_b, 0x54_b, 0x1c_b, 0x83_b, 0x5b_b, 0x9a_b, 0x2f_b, 0xc1_b, 0x6f_b,
            0xf1_b, 0xe5_b, 0xbe_b, 0x34_b, 0xb8_b, 0x96_b, 0xd8_b, 0x39_b, 0x4c_b, 0x6c_b, 0x31_b,
            0x73_b, 0x09_b};
        CHECK(block == correctlyEncodedBlock);

        std::ranges::copy(originalMessage, block.begin());
        sts1cobcsw::tc::Encode(block);
        correctlyEncodedBlock = std::array{
            0xff_b, 0x38_b, 0x9c_b, 0x59_b, 0x6c_b, 0xec_b, 0x00_b, 0xf2_b, 0x64_b, 0x80_b, 0x25_b,
            0xaa_b, 0x3d_b, 0x53_b, 0x06_b, 0xcf_b, 0x42_b, 0xb9_b, 0xa9_b, 0xbd_b, 0x5a_b, 0xd7_b,
            0xd1_b, 0xfa_b, 0x7e_b, 0xc5_b, 0x22_b, 0xcf_b, 0xe4_b, 0x9b_b, 0x4e_b, 0x22_b, 0xde_b,
            0x52_b, 0x1e_b, 0x97_b, 0xf5_b, 0xf7_b, 0x2b_b, 0xcd_b, 0xf1_b, 0x3b_b, 0x75_b, 0x69_b,
            0x4e_b, 0x91_b, 0x3f_b, 0xaf_b, 0x95_b, 0x60_b, 0x45_b, 0x6f_b, 0xa9_b, 0xb0_b, 0xb9_b,
            0xed_b, 0xf5_b, 0x81_b, 0x4b_b, 0x92_b, 0xcd_b, 0x31_b, 0x9e_b, 0x44_b, 0xbc_b, 0xa7_b,
            0x3b_b, 0x2a_b, 0xe7_b, 0xe1_b, 0x5d_b, 0x92_b, 0xfa_b, 0x6d_b, 0xf4_b, 0xcf_b, 0x89_b,
            0x35_b, 0x6d_b, 0x4e_b, 0x1a_b, 0xf3_b, 0xbc_b, 0xea_b, 0x6f_b, 0x5e_b, 0x49_b, 0xe2_b,
            0xc3_b, 0x29_b, 0xb9_b, 0x08_b, 0xbe_b, 0x44_b, 0x1e_b, 0xa8_b, 0x99_b, 0xad_b, 0x90_b,
            0xb0_b, 0x23_b, 0x2d_b, 0x51_b, 0xcc_b, 0x0c_b, 0x20_b, 0x17_b, 0x62_b, 0xe6_b, 0x9d_b,
            0x28_b, 0x6d_b, 0xe5_b, 0x34_b, 0xaf_b, 0x01_b, 0x02_b, 0x63_b, 0x49_b, 0x1c_b, 0x4e_b,
            0x98_b, 0xbc_b, 0xdc_b, 0xb8_b, 0x4f_b, 0xff_b, 0x90_b, 0x73_b, 0x18_b, 0x67_b, 0x25_b,
            0x0a_b, 0x15_b, 0xe9_b, 0xd1_b, 0x40_b, 0x1b_b, 0x70_b, 0x98_b, 0x99_b, 0x6d_b, 0x02_b,
            0x8a_b, 0xba_b, 0x1a_b, 0x28_b, 0x77_b, 0x78_b, 0xb9_b, 0xe8_b, 0x41_b, 0xf5_b, 0x5a_b,
            0x17_b, 0xd4_b, 0x14_b, 0xf8_b, 0x9d_b, 0x40_b, 0x47_b, 0x92_b, 0x69_b, 0xee_b, 0xb9_b,
            0x85_b, 0x78_b, 0x0a_b, 0x39_b, 0x8c_b, 0x5e_b, 0x8d_b, 0x87_b, 0x6c_b, 0xb6_b, 0xa5_b,
            0xe5_b, 0xa6_b, 0xc7_b, 0x7a_b, 0x6c_b, 0xed_b, 0x4b_b, 0x1b_b, 0x63_b, 0x3e_b, 0xa0_b,
            0x24_b, 0xac_b, 0x77_b, 0xb9_b, 0x00_b, 0x0e_b, 0xa6_b, 0x54_b, 0x59_b, 0xfe_b, 0x84_b,
            0x7b_b, 0x9c_b, 0xea_b, 0x82_b, 0x22_b, 0x87_b, 0x9b_b, 0x4f_b, 0xfe_b, 0xdb_b, 0x7a_b,
            0xff_b, 0x39_b, 0x40_b, 0x64_b, 0x64_b, 0x2d_b, 0x8e_b, 0x6f_b, 0xd7_b, 0xef_b, 0xe5_b,
            0xfd_b, 0x49_b, 0xd1_b, 0x30_b, 0x67_b, 0x5d_b, 0xf0_b, 0x61_b, 0x0a_b, 0x45_b, 0x05_b,
            0x91_b, 0xbf_b, 0x1e_b, 0x52_b, 0x57_b, 0xf7_b, 0xfc_b, 0x9d_b, 0x2b_b, 0xde_b, 0x8c_b,
            0xc0_b, 0xdb_b, 0x8e_b, 0x08_b, 0xd1_b, 0x20_b, 0xbe_b, 0x5d_b, 0xe2_b, 0x5a_b, 0x14_b,
            0xae_b, 0x4f_b};
        CHECK(block == correctlyEncodedBlock);
    }
}
