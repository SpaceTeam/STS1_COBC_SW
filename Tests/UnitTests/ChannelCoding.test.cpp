#include <Sts1CobcSw/ChannelCoding/ChannelCoding.hpp>
#include <Sts1CobcSw/ChannelCoding/ReedSolomon.hpp>
#include <Sts1CobcSw/ChannelCoding/Scrambler.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>


namespace rs = sts1cobcsw::rs;

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


TEST_CASE("Reed-Solomon")
{
    auto block = std::array<Byte, rs::blockLength>{};
    auto message = std::array<Byte, rs::messageLength>{};
    auto paritySymbols = std::array<Byte, rs::nParitySymbols>{};
    static constexpr auto originalMessage = GenerateSequentialBytes<rs::messageLength>();

    SECTION("Encode() computes the correct parity symbols")
    {
        std::ranges::copy(originalMessage, block.begin());
        rs::Encode(std::span(block).first<rs::messageLength>(),
                   std::span(block).last<rs::nParitySymbols>());
        static constexpr auto correctParitySymbols =
            std::array{79_b,  251_b, 146_b, 221_b, 85_b,  126_b, 198_b, 127_b, 39_b,  251_b, 137_b,
                       130_b, 207_b, 88_b,  248_b, 253_b, 2_b,   138_b, 209_b, 23_b,  252_b, 239_b,
                       107_b, 39_b,  147_b, 208_b, 65_b,  136_b, 38_b,  87_b,  134_b, 81_b};
        std::ranges::copy(std::span(block).last<rs::nParitySymbols>(), paritySymbols.begin());
        CHECK(paritySymbols == correctParitySymbols);
    }

    SECTION("Up to nParitySymbols / 2 errors can be corrected")
    {
        static constexpr auto nErrors = sts1cobcsw::nParitySymbols / 2U;

        std::ranges::copy(originalMessage, block.begin());
        rs::Encode(std::span(block).first<rs::messageLength>(),
                   std::span(block).last<rs::nParitySymbols>());
        for(auto i = 0U; i < nErrors; ++i)
        {
            message[i] ^= 0xFF_b;
        }
        rs::Decode(block);
        std::copy_n(block.begin(), message.size(), message.begin());
        CHECK(message == originalMessage);

        for(auto i = 0U; i < nErrors; ++i)
        {
            message[5 * i + 2] ^= 0xFF_b;
        }
        rs::Decode(block);
        std::copy_n(block.begin(), message.size(), message.begin());
        CHECK(message == originalMessage);

        for(auto i = 0U; i < nErrors + 1U; ++i)
        {
            message[i] ^= 0xFF_b;
        }
        rs::Decode(block);
        std::copy_n(block.begin(), message.size(), message.begin());
        CHECK(message == originalMessage);
    }
}


TEST_CASE("Scrambler")
{
    static constexpr auto originalData = GenerateSequentialBytes<rs::blockLength>();

    SECTION("Unscramble() is the inverse of Scramble()")
    {
        auto data = originalData;
        sts1cobcsw::tc::Scramble(data);
        sts1cobcsw::tc::Unscramble(data);
        CHECK(data == originalData);

        sts1cobcsw::tm::Scramble(data);
        sts1cobcsw::tm::Unscramble(data);
        CHECK(data == originalData);
    }

    SECTION("Regression tests")
    {
        auto data = originalData;
        sts1cobcsw::tc::Unscramble(data);
        static constexpr auto correctlyUnscrambledData = std::array{
            255_b, 56_b,  156_b, 89_b,  108_b, 236_b, 0_b,   242_b, 100_b, 128_b, 37_b,  170_b,
            61_b,  83_b,  6_b,   207_b, 66_b,  185_b, 169_b, 189_b, 90_b,  215_b, 209_b, 250_b,
            126_b, 197_b, 34_b,  207_b, 228_b, 155_b, 78_b,  34_b,  222_b, 82_b,  30_b,  151_b,
            245_b, 247_b, 43_b,  205_b, 241_b, 59_b,  117_b, 105_b, 78_b,  145_b, 63_b,  175_b,
            149_b, 96_b,  69_b,  111_b, 169_b, 176_b, 185_b, 237_b, 245_b, 129_b, 75_b,  146_b,
            205_b, 49_b,  158_b, 68_b,  188_b, 167_b, 59_b,  42_b,  231_b, 225_b, 93_b,  146_b,
            250_b, 109_b, 244_b, 207_b, 137_b, 53_b,  109_b, 78_b,  26_b,  243_b, 188_b, 234_b,
            111_b, 94_b,  73_b,  226_b, 195_b, 41_b,  185_b, 8_b,   190_b, 68_b,  30_b,  168_b,
            153_b, 173_b, 144_b, 176_b, 35_b,  45_b,  81_b,  204_b, 12_b,  32_b,  23_b,  98_b,
            230_b, 157_b, 40_b,  109_b, 229_b, 52_b,  175_b, 1_b,   2_b,   99_b,  73_b,  28_b,
            78_b,  152_b, 188_b, 220_b, 184_b, 79_b,  255_b, 144_b, 115_b, 24_b,  103_b, 37_b,
            10_b,  21_b,  233_b, 209_b, 64_b,  27_b,  112_b, 152_b, 153_b, 109_b, 2_b,   138_b,
            186_b, 26_b,  40_b,  119_b, 120_b, 185_b, 232_b, 65_b,  245_b, 90_b,  23_b,  212_b,
            20_b,  248_b, 157_b, 64_b,  71_b,  146_b, 105_b, 238_b, 185_b, 133_b, 120_b, 10_b,
            57_b,  140_b, 94_b,  141_b, 135_b, 108_b, 182_b, 165_b, 229_b, 166_b, 199_b, 122_b,
            108_b, 237_b, 75_b,  27_b,  99_b,  62_b,  160_b, 36_b,  172_b, 119_b, 185_b, 0_b,
            14_b,  166_b, 84_b,  89_b,  254_b, 132_b, 123_b, 156_b, 234_b, 130_b, 34_b,  135_b,
            155_b, 79_b,  254_b, 219_b, 122_b, 255_b, 57_b,  64_b,  100_b, 100_b, 45_b,  142_b,
            111_b, 215_b, 239_b, 229_b, 253_b, 73_b,  209_b, 160_b, 124_b, 46_b,  207_b, 215_b,
            144_b, 102_b, 156_b, 81_b,  172_b, 126_b, 58_b,  115_b, 67_b,  233_b, 142_b, 198_b,
            164_b, 172_b, 37_b,  212_b, 149_b, 150_b, 0_b,   68_b,  150_b, 229_b, 144_b, 135_b,
            191_b, 213_b, 224_b};
        CHECK(data == correctlyUnscrambledData);

        data = originalData;
        sts1cobcsw::tm::Scramble(data);
        static constexpr auto correctlyScrambledData = std::array{
            255_b, 73_b,  12_b,  195_b, 158_b, 8_b,   118_b, 187_b, 134_b, 37_b,  153_b, 166_b,
            171_b, 186_b, 72_b,  193_b, 74_b,  134_b, 111_b, 223_b, 38_b,  183_b, 169_b, 41_b,
            18_b,  9_b,   235_b, 147_b, 136_b, 208_b, 244_b, 174_b, 222_b, 177_b, 63_b,  162_b,
            16_b,  63_b,  199_b, 94_b,  52_b,  112_b, 13_b,  112_b, 99_b,  67_b,  163_b, 179_b,
            133_b, 31_b,  201_b, 171_b, 81_b,  112_b, 72_b,  75_b,  44_b,  24_b,  217_b, 42_b,
            21_b,  166_b, 235_b, 92_b,  189_b, 97_b,  121_b, 65_b,  44_b,  112_b, 132_b, 181_b,
            112_b, 251_b, 4_b,   253_b, 210_b, 144_b, 85_b,  118_b, 58_b,  12_b,  165_b, 99_b,
            158_b, 223_b, 170_b, 175_b, 112_b, 26_b,  156_b, 121_b, 15_b,  106_b, 244_b, 152_b,
            154_b, 33_b,  20_b,  103_b, 180_b, 14_b,  227_b, 131_b, 25_b,  13_b,  247_b, 6_b,
            81_b,  215_b, 88_b,  29_b,  164_b, 202_b, 156_b, 18_b,  225_b, 96_b,  143_b, 135_b,
            40_b,  254_b, 246_b, 63_b,  218_b, 18_b,  43_b,  240_b, 116_b, 1_b,   110_b, 138_b,
            36_b,  82_b,  141_b, 79_b,  106_b, 64_b,  176_b, 81_b,  247_b, 249_b, 226_b, 106_b,
            57_b,  230_b, 78_b,  80_b,  190_b, 190_b, 101_b, 119_b, 57_b,  150_b, 130_b, 18_b,
            208_b, 67_b,  53_b,  128_b, 73_b,  160_b, 122_b, 176_b, 229_b, 11_b,  177_b, 54_b,
            109_b, 59_b,  223_b, 31_b,  90_b,  69_b,  119_b, 100_b, 226_b, 94_b,  11_b,  53_b,
            224_b, 226_b, 81_b,  118_b, 250_b, 167_b, 139_b, 169_b, 37_b,  0_b,   232_b, 128_b,
            18_b,  194_b, 114_b, 229_b, 71_b,  153_b, 233_b, 228_b, 67_b,  237_b, 33_b,  162_b,
            33_b,  28_b,  125_b, 89_b,  117_b, 14_b,  161_b, 223_b, 124_b, 122_b, 25_b,  85_b,
            92_b,  229_b, 184_b, 254_b, 239_b, 167_b, 114_b, 160_b, 68_b,  230_b, 130_b, 174_b,
            226_b, 93_b,  184_b, 160_b, 254_b, 160_b, 60_b,  56_b,  55_b,  78_b,  137_b, 194_b,
            187_b, 79_b,  20_b,  234_b, 165_b, 170_b, 105_b, 242_b, 240_b, 129_b, 62_b,  177_b,
            154_b, 8_b,   166_b};
        CHECK(data == correctlyScrambledData);
    }
}


TEST_CASE("Channel coding")
{
    auto block = std::array<Byte, rs::blockLength>{};
    auto message = std::array<Byte, rs::messageLength>{};
    static constexpr auto originalMessage = GenerateSequentialBytes<rs::messageLength>();

    SECTION("Decoding is the inverse of encoding")
    {
        std::ranges::copy(originalMessage, block.begin());
        sts1cobcsw::tc::Encode(block);
        sts1cobcsw::tc::Decode(block);
        std::copy_n(block.begin(), message.size(), message.begin());
        CHECK(message == originalMessage);

        std::ranges::copy(originalMessage, block.begin());
        sts1cobcsw::tm::Encode(block);
        sts1cobcsw::tm::Decode(block);
        std::copy_n(block.begin(), message.size(), message.begin());
        CHECK(message == originalMessage);
    }

    SECTION("Up to nParitySymbols / 2 errors can be corrected")
    {
        static constexpr auto nErrors = sts1cobcsw::nParitySymbols / 2U;

        std::ranges::copy(originalMessage, block.begin());
        sts1cobcsw::tm::Encode(block);
        for(auto i = 0U; i < nErrors; ++i)
        {
            block[i] ^= 0xFF_b;
        }
        sts1cobcsw::tm::Decode(block);
        std::copy_n(block.begin(), message.size(), message.begin());
        CHECK(message == originalMessage);

        std::ranges::copy(originalMessage, block.begin());
        sts1cobcsw::tc::Encode(block);
        for(auto i = 0U; i < nErrors; ++i)
        {
            block[i] ^= 0xFF_b;
        }
        sts1cobcsw::tc::Decode(block);
        std::copy_n(block.begin(), message.size(), message.begin());
        CHECK(message == originalMessage);
    }


    SECTION("Regression tests")
    {
        std::ranges::copy(originalMessage, block.begin());
        sts1cobcsw::tm::Encode(block);
        static constexpr auto correctlyEncodedBlock = std::array{
            255_b, 73_b,  12_b,  195_b, 158_b, 8_b,   118_b, 187_b, 134_b, 37_b,  153_b, 166_b,
            171_b, 186_b, 72_b,  193_b, 74_b,  134_b, 111_b, 223_b, 38_b,  183_b, 169_b, 41_b,
            18_b,  9_b,   235_b, 147_b, 136_b, 208_b, 244_b, 174_b, 222_b, 177_b, 63_b,  162_b,
            16_b,  63_b,  199_b, 94_b,  52_b,  112_b, 13_b,  112_b, 99_b,  67_b,  163_b, 179_b,
            133_b, 31_b,  201_b, 171_b, 81_b,  112_b, 72_b,  75_b,  44_b,  24_b,  217_b, 42_b,
            21_b,  166_b, 235_b, 92_b,  189_b, 97_b,  121_b, 65_b,  44_b,  112_b, 132_b, 181_b,
            112_b, 251_b, 4_b,   253_b, 210_b, 144_b, 85_b,  118_b, 58_b,  12_b,  165_b, 99_b,
            158_b, 223_b, 170_b, 175_b, 112_b, 26_b,  156_b, 121_b, 15_b,  106_b, 244_b, 152_b,
            154_b, 33_b,  20_b,  103_b, 180_b, 14_b,  227_b, 131_b, 25_b,  13_b,  247_b, 6_b,
            81_b,  215_b, 88_b,  29_b,  164_b, 202_b, 156_b, 18_b,  225_b, 96_b,  143_b, 135_b,
            40_b,  254_b, 246_b, 63_b,  218_b, 18_b,  43_b,  240_b, 116_b, 1_b,   110_b, 138_b,
            36_b,  82_b,  141_b, 79_b,  106_b, 64_b,  176_b, 81_b,  247_b, 249_b, 226_b, 106_b,
            57_b,  230_b, 78_b,  80_b,  190_b, 190_b, 101_b, 119_b, 57_b,  150_b, 130_b, 18_b,
            208_b, 67_b,  53_b,  128_b, 73_b,  160_b, 122_b, 176_b, 229_b, 11_b,  177_b, 54_b,
            109_b, 59_b,  223_b, 31_b,  90_b,  69_b,  119_b, 100_b, 226_b, 94_b,  11_b,  53_b,
            224_b, 226_b, 81_b,  118_b, 250_b, 167_b, 139_b, 169_b, 37_b,  0_b,   232_b, 128_b,
            18_b,  194_b, 114_b, 229_b, 71_b,  153_b, 233_b, 228_b, 67_b,  237_b, 33_b,  162_b,
            33_b,  28_b,  125_b, 89_b,  117_b, 14_b,  161_b, 223_b, 124_b, 122_b, 25_b,  85_b,
            92_b,  229_b, 184_b, 254_b, 239_b, 167_b, 114_b, 48_b,  95_b,  149_b, 189_b, 24_b,
            120_b, 126_b, 33_b,  96_b,  237_b, 192_b, 84_b,  28_b,  131_b, 91_b,  154_b, 47_b,
            193_b, 111_b, 241_b, 229_b, 190_b, 52_b,  184_b, 150_b, 216_b, 57_b,  76_b,  108_b,
            49_b,  115_b, 9_b};
        CHECK(block == correctlyEncodedBlock);

        static constexpr auto encodedBlock = GenerateSequentialBytes<rs::blockLength>();
        block = encodedBlock;
        sts1cobcsw::tc::Decode(block);
        static constexpr auto correctMessage = std::array{
            255_b, 56_b,  156_b, 89_b,  108_b, 236_b, 0_b,   242_b, 100_b, 128_b, 37_b,  170_b,
            61_b,  83_b,  6_b,   207_b, 66_b,  185_b, 169_b, 189_b, 90_b,  215_b, 209_b, 250_b,
            126_b, 197_b, 34_b,  207_b, 228_b, 155_b, 78_b,  34_b,  222_b, 82_b,  30_b,  151_b,
            245_b, 247_b, 43_b,  205_b, 241_b, 59_b,  117_b, 105_b, 78_b,  145_b, 63_b,  175_b,
            149_b, 96_b,  69_b,  111_b, 169_b, 176_b, 185_b, 237_b, 245_b, 129_b, 75_b,  146_b,
            205_b, 49_b,  158_b, 68_b,  188_b, 167_b, 59_b,  42_b,  231_b, 225_b, 93_b,  146_b,
            250_b, 109_b, 244_b, 207_b, 137_b, 53_b,  109_b, 78_b,  26_b,  243_b, 188_b, 234_b,
            111_b, 94_b,  73_b,  226_b, 195_b, 41_b,  185_b, 8_b,   190_b, 68_b,  30_b,  168_b,
            153_b, 173_b, 144_b, 176_b, 35_b,  45_b,  81_b,  204_b, 12_b,  32_b,  23_b,  98_b,
            230_b, 157_b, 40_b,  109_b, 229_b, 52_b,  175_b, 1_b,   2_b,   99_b,  73_b,  28_b,
            78_b,  152_b, 188_b, 220_b, 184_b, 79_b,  255_b, 144_b, 115_b, 24_b,  103_b, 37_b,
            10_b,  21_b,  233_b, 209_b, 64_b,  27_b,  112_b, 152_b, 153_b, 109_b, 2_b,   138_b,
            186_b, 26_b,  40_b,  119_b, 120_b, 185_b, 232_b, 65_b,  245_b, 90_b,  23_b,  212_b,
            20_b,  248_b, 157_b, 64_b,  71_b,  146_b, 105_b, 238_b, 185_b, 133_b, 120_b, 10_b,
            57_b,  140_b, 94_b,  141_b, 135_b, 108_b, 182_b, 165_b, 229_b, 166_b, 199_b, 122_b,
            108_b, 237_b, 75_b,  27_b,  99_b,  62_b,  160_b, 36_b,  172_b, 119_b, 185_b, 0_b,
            14_b,  166_b, 84_b,  89_b,  254_b, 132_b, 123_b, 156_b, 234_b, 130_b, 34_b,  135_b,
            155_b, 79_b,  254_b, 219_b, 122_b, 255_b, 57_b,  64_b,  100_b, 100_b, 45_b,  142_b,
            111_b, 215_b, 239_b, 229_b, 253_b, 73_b,  209_b};
        std::copy_n(block.begin(), message.size(), message.begin());
        CHECK(message == correctMessage);
    }
}
