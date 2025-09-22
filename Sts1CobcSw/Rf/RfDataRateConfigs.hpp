//! @file
//! @brief Contains RX and TX configurations for different DataRates for Si4463 (created by WDS3)
//!
//! example: (RX 1.200)
//! #define RF_MODEM_DSA_CTRL1_5 0x11, 0x20, 0x05, 0x5B,  0x40, 0x04, 0x06, 0x78, 0x20
//!                               ^     ^     ^     ^      ^
//!                               ^-----|-----|-----|------|--- SET_PROPERTY command byte = 0x11
//!                                     ^-----|-----|------|--- Property Group (0x20 = MODEM group)
//!                                           ^-----|------|--- length (max. = 12 = 0x0C)
//!                                                 ^------|--- startindex = 0x5B
//!                                                        ^--- data[0] (first data value)

#pragma once


#include <Sts1CobcSw/Serial/Byte.hpp>

#include <array>
#include <climits>
#include <cstddef>
#include <cstdint>


namespace sts1cobcsw::rf
{
using sts1cobcsw::operator""_b;


// Max. number of properties that can be set in a single command
inline constexpr auto maxNProperties = 12;


enum class PropertyGroup : std::uint8_t
{
    global = 0x00,       //
    intCtl = 0x01,       // Interrupt control
    frrCtl = 0x02,       // Fast response register control
    preamble = 0x10,     //
    sync = 0x11,         // Sync word
    pkt = 0x12,          // Packet
    modem = 0x20,        // Selects type of modulation. TX: also selects source of modulation.
    modemChflt = 0x21,   // Filter coefficients
    pa = 0x22,           // Power amplifier
    synth = 0x23,        //
    match = 0x30,        //
    freqControl = 0x40,  //
    rxHop = 0x50,        //
    pti = 0xF0           // Packet trace interface
};


template<PropertyGroup propertyGroup, Byte propertyStartIndex, std::size_t nProperties>
    requires(nProperties <= maxNProperties)
class Properties
{
public:
    static constexpr auto group = propertyGroup;
    static constexpr auto startIndex = propertyStartIndex;

    template<std::size_t size>
        requires(size == nProperties)
    explicit constexpr Properties(std::array<Byte, size> const & values) : values_(values)
    {}

    [[nodiscard]] auto GetValues() const -> std::array<Byte, nProperties> const &
    {
        return values_;
    }


private:
    std::array<Byte, nProperties> values_ = {};
};


// clang-format off
// NOLINTBEGIN(*identifier-naming, *magic-numbers)
using MODEM_DATA_RATE_t             = Properties<PropertyGroup::modem,      0x03_b,  4>;
using MODEM_FREQ_DEV_t              = Properties<PropertyGroup::modem,      0x0b_b,  2>;
using MODEM_DECIMATION_CFG1_t       = Properties<PropertyGroup::modem,      0x1e_b,  3>;
using MODEM_BCR_OSR_t               = Properties<PropertyGroup::modem,      0x23_b,  6>;
using MODEM_AFC_WAIT_t              = Properties<PropertyGroup::modem,      0x2d_b,  5>;
using MODEM_AGC_RFPD_DECAY_t        = Properties<PropertyGroup::modem,      0x39_b,  2>;
using MODEM_OOK_PDTC_t              = Properties<PropertyGroup::modem,      0x40_b,  1>;
using MODEM_RAW_EYE_t               = Properties<PropertyGroup::modem,      0x46_b,  2>;
using MODEM_SPIKE_DET_t             = Properties<PropertyGroup::modem,      0x54_b,  1>;
using MODEM_DSA_QUAL_t              = Properties<PropertyGroup::modem,      0x5d_b,  1>;
using MODEM_CHFLT_RX1_CHFLT_COE_t   = Properties<PropertyGroup::modemChflt, 0x00_b, 12>;
using MODEM_CHFLT_RX1_CHFLT_COE_2_t = Properties<PropertyGroup::modemChflt, 0x0C_b, 12>;
using MODEM_CHFLT_RX2_CHFLT_COE_t   = Properties<PropertyGroup::modemChflt, 0x18_b, 12>;
using PREAMBLE_TX_LENGTH_t          = Properties<PropertyGroup::preamble,   0x00_b,  1>;
// NOLINTEND(*identifier-naming, *magic-numbers)
// clang-format on


// NOLINTBEGIN(*identifier-naming)
struct DataRateConfig
{
    std::uint32_t dataRate = 0U;
    MODEM_DATA_RATE_t MODEM_DATA_RATE;
    MODEM_FREQ_DEV_t MODEM_FREQ_DEV;
    MODEM_DECIMATION_CFG1_t MODEM_DECIMATION_CFG1;
    MODEM_BCR_OSR_t MODEM_BCR_OSR;
    MODEM_AFC_WAIT_t MODEM_AFC_WAIT;
    MODEM_AGC_RFPD_DECAY_t MODEM_AGC_RFPD_DECAY;
    MODEM_OOK_PDTC_t MODEM_OOK_PDTC;
    MODEM_RAW_EYE_t MODEM_RAW_EYE;
    MODEM_SPIKE_DET_t MODEM_SPIKE_DET;
    MODEM_DSA_QUAL_t MODEM_DSA_QUAL;
    MODEM_CHFLT_RX1_CHFLT_COE_t MODEM_CHFLT_RX1_CHFLT_COE;
    MODEM_CHFLT_RX1_CHFLT_COE_2_t MODEM_CHFLT_RX1_CHFLT_COE_2;
    MODEM_CHFLT_RX2_CHFLT_COE_t MODEM_CHFLT_RX2_CHFLT_COE;
    // 500 µs startup time required by TX amp -> (0.0005 * baudrate / 8) rounded up.
    PREAMBLE_TX_LENGTH_t PREAMBLE_TX_LENGTH;
};
// NOLINTEND(*identifier-naming)


constexpr auto ComputePreambleLength(uint32_t dataRate) -> Byte
{
    // TX amplifier requires 500 µs startup time -> ((500e-6 * dataRate) / 8) rounded up
    constexpr auto startupTimeUs = 500;
    constexpr auto divisor = CHAR_BIT * 1'000'000;
    return static_cast<Byte>((startupTimeUs * dataRate + divisor - 1) / divisor);
}


constexpr auto dataRateConfig1200 = DataRateConfig{
    1200,
    // clang-format off
    MODEM_DATA_RATE_t(std::array{            0x00_b, 0xBB_b, 0x80_b, 0x05_b}),
    MODEM_FREQ_DEV_t(std::array{             0x00_b, 0x30_b}),
    MODEM_DECIMATION_CFG1_t(std::array{      0xF0_b, 0x20_b, 0x0C_b}),
    MODEM_BCR_OSR_t(std::array{              0xA9_b, 0x03_b, 0x06_b, 0x55_b, 0x03_b, 0x08_b}),
    MODEM_AFC_WAIT_t(std::array{             0x12_b, 0x80_b, 0x0C_b, 0x03_b, 0xB5_b}),
    MODEM_AGC_RFPD_DECAY_t(std::array{       0x25_b, 0x25_b}),
    MODEM_OOK_PDTC_t(std::array{             0x29_b}),
    MODEM_RAW_EYE_t(std::array{              0x00_b,0x7B_b}),
    MODEM_SPIKE_DET_t(std::array{            0x03_b}),
    MODEM_DSA_QUAL_t(std::array{             0x06_b}),
    MODEM_CHFLT_RX1_CHFLT_COE_t(std::array{  0xCC_b, 0xA1_b, 0x30_b, 0xA0_b, 0x21_b, 0xD1_b,
                                             0xB9_b, 0xC9_b, 0xEA_b, 0x05_b, 0x12_b, 0x11_b}),
    MODEM_CHFLT_RX1_CHFLT_COE_2_t(std::array{0x0A_b, 0x04_b, 0x15_b, 0xFC_b, 0x03_b, 0x00_b,
                                             0xCC_b, 0xA1_b, 0x30_b, 0xA0_b, 0x21_b, 0xD1_b}),
    MODEM_CHFLT_RX2_CHFLT_COE_t(std::array{  0xB9_b, 0xC9_b, 0xEA_b, 0x05_b, 0x12_b, 0x11_b,
                                             0x0A_b, 0x04_b, 0x15_b, 0xFC_b, 0x03_b, 0x00_b}),
    // clang-format on
    PREAMBLE_TX_LENGTH_t(std::array{ComputePreambleLength(1200)}),
};

constexpr auto dataRateConfig2400 = DataRateConfig{
    2400,
    // clang-format off
    MODEM_DATA_RATE_t(std::array{            0x01_b, 0x77_b, 0x00_b, 0x05_b}),
    MODEM_FREQ_DEV_t(std::array{             0x00_b, 0x61_b}),
    MODEM_DECIMATION_CFG1_t(std::array{      0xF0_b, 0x20_b, 0x0C_b}),
    MODEM_BCR_OSR_t(std::array{              0x55_b, 0x06_b, 0x0C_b, 0xAB_b, 0x06_b, 0x06_b}),
    MODEM_AFC_WAIT_t(std::array{             0x12_b, 0x80_b, 0x18_b, 0x02_b, 0x4A_b}),
    MODEM_AGC_RFPD_DECAY_t(std::array{       0x13_b, 0x13_b}),
    MODEM_OOK_PDTC_t(std::array{             0x28_b}),
    MODEM_RAW_EYE_t(std::array{              0x00_b,0xF5_b}),
    MODEM_SPIKE_DET_t(std::array{            0x03_b}),
    MODEM_DSA_QUAL_t(std::array{             0x08_b}),
    MODEM_CHFLT_RX1_CHFLT_COE_t(std::array{  0xFF_b, 0xBA_b, 0x0F_b, 0x51_b, 0xCF_b, 0xA9_b,
                                             0xC9_b, 0xFC_b, 0x1B_b, 0x1E_b, 0x0F_b, 0x01_b}),
    MODEM_CHFLT_RX1_CHFLT_COE_2_t(std::array{0xFC_b, 0xFD_b, 0x15_b, 0xFF_b, 0x00_b, 0x0F_b,
                                             0xFF_b, 0xBA_b, 0x0F_b, 0x51_b, 0xCF_b, 0xA9_b}),
    MODEM_CHFLT_RX2_CHFLT_COE_t(std::array{  0xC9_b, 0xFC_b, 0x1B_b, 0x1E_b, 0x0F_b, 0x01_b,
                                             0xFC_b, 0xFD_b, 0x15_b, 0xFF_b, 0x00_b, 0x0F_b}),
    // clang-format on
    PREAMBLE_TX_LENGTH_t(std::array{ComputePreambleLength(2400)}),
};

constexpr auto dataRateConfig4800 = DataRateConfig{
    4800,
    // clang-format off
    MODEM_DATA_RATE_t(std::array{            0x02_b, 0xEE_b, 0x00_b, 0x05_b}),
    MODEM_FREQ_DEV_t(std::array{             0x00_b, 0xC2_b}),
    MODEM_DECIMATION_CFG1_t(std::array{      0xB0_b, 0x20_b, 0x0C_b}),
    MODEM_BCR_OSR_t(std::array{              0x55_b, 0x06_b, 0x0C_b, 0xAB_b, 0x06_b, 0x06_b}),
    MODEM_AFC_WAIT_t(std::array{             0x12_b, 0x80_b, 0x30_b, 0x01_b, 0xDA_b}),
    MODEM_AGC_RFPD_DECAY_t(std::array{       0x13_b, 0x13_b}),
    MODEM_OOK_PDTC_t(std::array{             0x28_b}),
    MODEM_RAW_EYE_t(std::array{              0x00_b,0xF5_b}),
    MODEM_SPIKE_DET_t(std::array{            0x03_b}),
    MODEM_DSA_QUAL_t(std::array{             0x07_b}),
    MODEM_CHFLT_RX1_CHFLT_COE_t(std::array{  0xCC_b, 0xA1_b, 0x30_b, 0xA0_b, 0x21_b, 0xD1_b,
                                             0xB9_b, 0xC9_b, 0xEA_b, 0x05_b, 0x12_b, 0x11_b}),
    MODEM_CHFLT_RX1_CHFLT_COE_2_t(std::array{0x0A_b, 0x04_b, 0x15_b, 0xFC_b, 0x03_b, 0x00_b,
                                             0xCC_b, 0xA1_b, 0x30_b, 0xA0_b, 0x21_b, 0xD1_b}),
    MODEM_CHFLT_RX2_CHFLT_COE_t(std::array{  0xB9_b, 0xC9_b, 0xEA_b, 0x05_b, 0x12_b, 0x11_b,
                                             0x0A_b, 0x04_b, 0x15_b, 0xFC_b, 0x03_b, 0x00_b}),
    // clang-format on
    PREAMBLE_TX_LENGTH_t(std::array{ComputePreambleLength(4800)}),
};

constexpr auto dataRateConfig9600 = DataRateConfig{
    9600,
    // clang-format off
    MODEM_DATA_RATE_t(std::array{            0x05_b, 0xDC_b, 0x00_b, 0x05_b}),
    MODEM_FREQ_DEV_t(std::array{             0x01_b, 0x83_b}),
    MODEM_DECIMATION_CFG1_t(std::array{      0x70_b, 0x20_b, 0x00_b}),
    MODEM_BCR_OSR_t(std::array{              0x55_b, 0x06_b, 0x0C_b, 0xAB_b, 0x06_b, 0x06_b}),
    MODEM_AFC_WAIT_t(std::array{             0x12_b, 0x80_b, 0x61_b, 0x01_b, 0xD5_b}),
    MODEM_AGC_RFPD_DECAY_t(std::array{       0x13_b, 0x13_b}),
    MODEM_OOK_PDTC_t(std::array{             0x28_b}),
    MODEM_RAW_EYE_t(std::array{              0x00_b,0xF5_b}),
    MODEM_SPIKE_DET_t(std::array{            0x03_b}),
    MODEM_DSA_QUAL_t(std::array{             0x07_b}),
    MODEM_CHFLT_RX1_CHFLT_COE_t(std::array{  0xCC_b, 0xA1_b, 0x30_b, 0xA0_b, 0x21_b, 0xD1_b,
                                             0xB9_b, 0xC9_b, 0xEA_b, 0x05_b, 0x12_b, 0x11_b}),
    MODEM_CHFLT_RX1_CHFLT_COE_2_t(std::array{0x0A_b, 0x04_b, 0x15_b, 0xFC_b, 0x03_b, 0x00_b,
                                             0xCC_b, 0xA1_b, 0x30_b, 0xA0_b, 0x21_b, 0xD1_b}),
    MODEM_CHFLT_RX2_CHFLT_COE_t(std::array{  0xB9_b, 0xC9_b, 0xEA_b, 0x05_b, 0x12_b, 0x11_b,
                                             0x0A_b, 0x04_b, 0x15_b, 0xFC_b, 0x03_b, 0x00_b}),
    // clang-format on
    PREAMBLE_TX_LENGTH_t(std::array{ComputePreambleLength(9600)}),
};

constexpr auto dataRateConfig19200 = DataRateConfig{
    19'200,
    // clang-format off
    MODEM_DATA_RATE_t(std::array{            0x0B_b, 0xB8_b, 0x00_b, 0x05_b}),
    MODEM_FREQ_DEV_t(std::array{             0x03_b, 0x06_b}),
    MODEM_DECIMATION_CFG1_t(std::array{      0x30_b, 0x20_b, 0x00_b}),
    MODEM_BCR_OSR_t(std::array{              0x55_b, 0x06_b, 0x0C_b, 0xAB_b, 0x06_b, 0x06_b}),
    MODEM_AFC_WAIT_t(std::array{             0x12_b, 0x80_b, 0xC2_b, 0x01_b, 0xD4_b}),
    MODEM_AGC_RFPD_DECAY_t(std::array{       0x13_b, 0x13_b}),
    MODEM_OOK_PDTC_t(std::array{             0x28_b}),
    MODEM_RAW_EYE_t(std::array{              0x00_b,0xF5_b}),
    MODEM_SPIKE_DET_t(std::array{            0x03_b}),
    MODEM_DSA_QUAL_t(std::array{             0x07_b}),
    MODEM_CHFLT_RX1_CHFLT_COE_t(std::array{  0xCC_b, 0xA1_b, 0x30_b, 0xA0_b, 0x21_b, 0xD1_b,
                                             0xB9_b, 0xC9_b, 0xEA_b, 0x05_b, 0x12_b, 0x11_b}),
    MODEM_CHFLT_RX1_CHFLT_COE_2_t(std::array{0x0A_b, 0x04_b, 0x15_b, 0xFC_b, 0x03_b, 0x00_b,
                                             0xCC_b, 0xA1_b, 0x30_b, 0xA0_b, 0x21_b, 0xD1_b}),
    MODEM_CHFLT_RX2_CHFLT_COE_t(std::array{  0xB9_b, 0xC9_b, 0xEA_b, 0x05_b, 0x12_b, 0x11_b,
                                             0x0A_b, 0x04_b, 0x15_b, 0xFC_b, 0x03_b, 0x00_b}),
    // clang-format on
    PREAMBLE_TX_LENGTH_t(std::array{ComputePreambleLength(19'200)}),
};

constexpr auto dataRateConfig38400 = DataRateConfig{
    38'400,
    // clang-format off
    MODEM_DATA_RATE_t(std::array{            0x0B_b, 0xB8_b, 0x00_b, 0x09_b}),
    MODEM_FREQ_DEV_t(std::array{             0x06_b, 0x0D_b}),
    MODEM_DECIMATION_CFG1_t(std::array{      0x20_b, 0x20_b, 0x00_b}),
    MODEM_BCR_OSR_t(std::array{              0x55_b, 0x06_b, 0x0C_b, 0xAB_b, 0x06_b, 0x06_b}),
    MODEM_AFC_WAIT_t(std::array{             0x12_b, 0x81_b, 0x83_b, 0x01_b, 0xD3_b}),
    MODEM_AGC_RFPD_DECAY_t(std::array{       0x13_b, 0x13_b}),
    MODEM_OOK_PDTC_t(std::array{             0x28_b}),
    MODEM_RAW_EYE_t(std::array{              0x00_b,0xF5_b}),
    MODEM_SPIKE_DET_t(std::array{            0x03_b}),
    MODEM_DSA_QUAL_t(std::array{             0x07_b}),
    MODEM_CHFLT_RX1_CHFLT_COE_t(std::array{  0xCC_b, 0xA1_b, 0x30_b, 0xA0_b, 0x21_b, 0xD1_b,
                                             0xB9_b, 0xC9_b, 0xEA_b, 0x05_b, 0x12_b, 0x11_b}),
    MODEM_CHFLT_RX1_CHFLT_COE_2_t(std::array{0x0A_b, 0x04_b, 0x15_b, 0xFC_b, 0x03_b, 0x00_b,
                                             0xCC_b, 0xA1_b, 0x30_b, 0xA0_b, 0x21_b, 0xD1_b}),
    MODEM_CHFLT_RX2_CHFLT_COE_t(std::array{  0xB9_b, 0xC9_b, 0xEA_b, 0x05_b, 0x12_b, 0x11_b,
                                             0x0A_b, 0x04_b, 0x15_b, 0xFC_b, 0x03_b, 0x00_b}),
    // clang-format on
    PREAMBLE_TX_LENGTH_t(std::array{ComputePreambleLength(38'400)}),
};

constexpr auto dataRateConfig57600 = DataRateConfig{
    57'600,
    // clang-format off
    MODEM_DATA_RATE_t(std::array{            0x11_b, 0x94_b, 0x00_b, 0x09_b}),
    MODEM_FREQ_DEV_t(std::array{             0x09_b, 0x13_b}),
    MODEM_DECIMATION_CFG1_t(std::array{      0x10_b, 0x10_b, 0x00_b}),
    MODEM_BCR_OSR_t(std::array{              0x4B_b, 0x06_b, 0xCE_b, 0x40_b, 0x06_b, 0xD4_b}),
    MODEM_AFC_WAIT_t(std::array{             0x12_b, 0x82_b, 0x45_b, 0x01_b, 0xCB_b}),
    MODEM_AGC_RFPD_DECAY_t(std::array{       0x10_b, 0x10_b}),
    MODEM_OOK_PDTC_t(std::array{             0x28_b}),
    MODEM_RAW_EYE_t(std::array{              0x01_b,0x14_b}),
    MODEM_SPIKE_DET_t(std::array{            0x04_b}),
    MODEM_DSA_QUAL_t(std::array{             0x08_b}),
    MODEM_CHFLT_RX1_CHFLT_COE_t(std::array{  0xFF_b, 0xC4_b, 0x30_b, 0x7F_b, 0xF5_b, 0xB5_b,
                                             0xB8_b, 0xDE_b, 0x05_b, 0x17_b, 0x16_b, 0x0C_b}),
    MODEM_CHFLT_RX1_CHFLT_COE_2_t(std::array{0x03_b, 0x00_b, 0x15_b, 0xFF_b, 0x00_b, 0x00_b,
                                             0xFF_b, 0xC4_b, 0x30_b, 0x7F_b, 0xF5_b, 0xB5_b}),
    MODEM_CHFLT_RX2_CHFLT_COE_t(std::array{  0xB8_b, 0xDE_b, 0x05_b, 0x17_b, 0x16_b, 0x0C_b,
                                             0x03_b, 0x00_b, 0x15_b, 0xFF_b, 0x00_b, 0x00_b}),
    // clang-format on
    PREAMBLE_TX_LENGTH_t(std::array{ComputePreambleLength(57'600)}),
};

constexpr auto dataRateConfig76800 = DataRateConfig{
    76'800,
    // clang-format off
    MODEM_DATA_RATE_t(std::array{            0x17_b, 0x70_b, 0x00_b, 0x09_b}),
    MODEM_FREQ_DEV_t(std::array{             0x0C_b, 0x19_b}),
    MODEM_DECIMATION_CFG1_t(std::array{      0x10_b, 0x20_b, 0x00_b}),
    MODEM_BCR_OSR_t(std::array{              0x55_b, 0x06_b, 0x0C_b, 0xAB_b, 0x06_b, 0x06_b}),
    MODEM_AFC_WAIT_t(std::array{             0x12_b, 0x83_b, 0x06_b, 0x01_b, 0xCF_b}),
    MODEM_AGC_RFPD_DECAY_t(std::array{       0x13_b, 0x13_b}),
    MODEM_OOK_PDTC_t(std::array{             0x28_b}),
    MODEM_RAW_EYE_t(std::array{              0x00_b,0xF5_b}),
    MODEM_SPIKE_DET_t(std::array{            0x03_b}),
    MODEM_DSA_QUAL_t(std::array{             0x07_b}),
    MODEM_CHFLT_RX1_CHFLT_COE_t(std::array{  0xCC_b, 0xA1_b, 0x30_b, 0xA0_b, 0x21_b, 0xD1_b,
                                             0xB9_b, 0xC9_b, 0xEA_b, 0x05_b, 0x12_b, 0x11_b}),
    MODEM_CHFLT_RX1_CHFLT_COE_2_t(std::array{0x0A_b, 0x04_b, 0x15_b, 0xFC_b, 0x03_b, 0x00_b,
                                             0xCC_b, 0xA1_b, 0x30_b, 0xA0_b, 0x21_b, 0xD1_b}),
    MODEM_CHFLT_RX2_CHFLT_COE_t(std::array{  0xB9_b, 0xC9_b, 0xEA_b, 0x05_b, 0x12_b, 0x11_b,
                                             0x0A_b, 0x04_b, 0x15_b, 0xFC_b, 0x03_b, 0x00_b}),
    // clang-format on
    PREAMBLE_TX_LENGTH_t(std::array{ComputePreambleLength(76'800)}),
};

constexpr auto dataRateConfig115200 = DataRateConfig{
    115'200,
    // clang-format off
    MODEM_DATA_RATE_t(std::array{            0x23_b, 0x28_b, 0x00_b, 0x09_b}),
    MODEM_FREQ_DEV_t(std::array{             0x12_b, 0x26_b}),
    MODEM_DECIMATION_CFG1_t(std::array{      0x00_b, 0x10_b, 0x00_b}),
    MODEM_BCR_OSR_t(std::array{              0x4B_b, 0x06_b, 0xCE_b, 0x40_b, 0x06_b, 0xD4_b}),
    MODEM_AFC_WAIT_t(std::array{             0x23_b, 0x89_b, 0x13_b, 0x00_b, 0xD4_b}),
    MODEM_AGC_RFPD_DECAY_t(std::array{       0x10_b, 0x10_b}),
    MODEM_OOK_PDTC_t(std::array{             0x28_b}),
    MODEM_RAW_EYE_t(std::array{              0x01_b,0x14_b}),
    MODEM_SPIKE_DET_t(std::array{            0x04_b}),
    MODEM_DSA_QUAL_t(std::array{             0x08_b}),
    MODEM_CHFLT_RX1_CHFLT_COE_t(std::array{  0xFF_b, 0xC4_b, 0x30_b, 0x7F_b, 0xF5_b, 0xB5_b,
                                             0xB8_b, 0xDE_b, 0x05_b, 0x17_b, 0x16_b, 0x0C_b}),
    MODEM_CHFLT_RX1_CHFLT_COE_2_t(std::array{0x03_b, 0x00_b, 0x15_b, 0xFF_b, 0x00_b, 0x00_b,
                                             0xFF_b, 0xC4_b, 0x30_b, 0x7F_b, 0xF5_b, 0xB5_b}),
    MODEM_CHFLT_RX2_CHFLT_COE_t(std::array{  0xB8_b, 0xDE_b, 0x05_b, 0x17_b, 0x16_b, 0x0C_b,
                                             0x03_b, 0x00_b, 0x15_b, 0xFF_b, 0x00_b, 0x00_b}),
    // clang-format on
    PREAMBLE_TX_LENGTH_t(std::array{ComputePreambleLength(115'200)}),
};
}
