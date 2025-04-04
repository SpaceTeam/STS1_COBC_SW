#include <Tests/HardwareSetup/RfLatchupProtection.hpp>

#include <Sts1CobcSw/Hal/GpioPin.hpp>
#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/Rf/Rf.hpp>
#include <Sts1CobcSw/RodosTime/RodosTime.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Utility/Span.hpp>

#include <strong_type/difference.hpp>

#include <rodos_no_using_namespace.h>

#include <array>
#include <cstdint>
#include <type_traits>


namespace sts1cobcsw
{
using RODOS::PRINTF;


// CCSDS test messages. RS(255,223) + CCSDS scrambling, 223 byte message, 8 byte preamble, 4
// byte sync word, once without and once with convolutional coding.
[[maybe_unused]] constexpr auto dataWithoutCc = std::to_array<Byte>(
    {0x33_b, 0x1a_b, 0xcf_b, 0xfc_b, 0x1d_b, 0x63_b, 0x91_b, 0xeb_b, 0x25_b, 0x1f_b, 0x23_b, 0xf8_b,
     0x39_b, 0xb3_b, 0xc9_b, 0x86_b, 0xad_b, 0xa7_b, 0xb7_b, 0x46_b, 0xce_b, 0x5a_b, 0x97_b, 0x7d_b,
     0xcc_b, 0x32_b, 0xa2_b, 0xbf_b, 0x3e_b, 0x0a_b, 0x10_b, 0xf1_b, 0x88_b, 0x94_b, 0xcd_b, 0xea_b,
     0xb1_b, 0xfe_b, 0x90_b, 0x1d_b, 0x81_b, 0x34_b, 0x1a_b, 0xe1_b, 0x79_b, 0x1c_b, 0x59_b, 0x27_b,
     0x5b_b, 0x4f_b, 0x6e_b, 0x8d_b, 0x9c_b, 0xb5_b, 0x2e_b, 0xfb_b, 0x98_b, 0x65_b, 0x45_b, 0x7e_b,
     0x7c_b, 0x14_b, 0x21_b, 0xe3_b, 0x11_b, 0x29_b, 0x9b_b, 0xd5_b, 0x63_b, 0xfd_b, 0x20_b, 0x3b_b,
     0x02_b, 0x68_b, 0x35_b, 0xc2_b, 0xf2_b, 0x38_b, 0xb2_b, 0x4e_b, 0xb6_b, 0x9e_b, 0xdd_b, 0x1b_b,
     0x39_b, 0x6a_b, 0x5d_b, 0xf7_b, 0x30_b, 0xca_b, 0x8a_b, 0xfc_b, 0xf8_b, 0x28_b, 0x43_b, 0xc6_b,
     0x22_b, 0x53_b, 0x37_b, 0xaa_b, 0xc7_b, 0xfa_b, 0x40_b, 0x76_b, 0x04_b, 0xd0_b, 0x6b_b, 0x85_b,
     0xe4_b, 0x71_b, 0x64_b, 0x9d_b, 0x6d_b, 0x3d_b, 0xba_b, 0x36_b, 0x72_b, 0xd4_b, 0xbb_b, 0xee_b,
     0x61_b, 0x95_b, 0x15_b, 0xf9_b, 0xf0_b, 0x50_b, 0x87_b, 0x8c_b, 0x44_b, 0xa6_b, 0x6f_b, 0x55_b,
     0x8f_b, 0xf4_b, 0x80_b, 0xec_b, 0x09_b, 0xa0_b, 0xd7_b, 0x0b_b, 0xc8_b, 0xe2_b, 0xc9_b, 0x3a_b,
     0xda_b, 0x7b_b, 0x74_b, 0x6c_b, 0xe5_b, 0xa9_b, 0x77_b, 0xdc_b, 0xc3_b, 0x2a_b, 0x2b_b, 0xf3_b,
     0xe0_b, 0xa1_b, 0x0f_b, 0x18_b, 0x89_b, 0x4c_b, 0xde_b, 0xab_b, 0x1f_b, 0xe9_b, 0x01_b, 0xd8_b,
     0x13_b, 0x41_b, 0xae_b, 0x17_b, 0x91_b, 0xc5_b, 0x92_b, 0x75_b, 0xb4_b, 0xf6_b, 0xe8_b, 0xd9_b,
     0xcb_b, 0x52_b, 0xef_b, 0xb9_b, 0x86_b, 0x54_b, 0x57_b, 0xe7_b, 0xc1_b, 0x42_b, 0x1e_b, 0x31_b,
     0x12_b, 0x99_b, 0xbd_b, 0x56_b, 0x3f_b, 0xd2_b, 0x03_b, 0xb0_b, 0x26_b, 0x83_b, 0x5c_b, 0x2f_b,
     0x23_b, 0x8b_b, 0x24_b, 0xeb_b, 0x69_b, 0xed_b, 0xd1_b, 0xb3_b, 0x96_b, 0xa5_b, 0xdf_b, 0x73_b,
     0x0c_b, 0xa8_b, 0xaf_b, 0xcf_b, 0x82_b, 0x84_b, 0x3c_b, 0x62_b, 0x25_b, 0x33_b, 0x7a_b, 0xac_b,
     0x44_b, 0x4e_b, 0xfc_b, 0xab_b, 0xee_b, 0x8b_b, 0x99_b, 0x27_b, 0x25_b, 0x90_b, 0x22_b, 0x69_b,
     0x76_b, 0x8a_b, 0x77_b, 0x68_b, 0xe0_b, 0xfe_b, 0x48_b, 0x97_b, 0x4d_b, 0x15_b, 0x68_b, 0x3e_b,
     0xbe_b, 0x82_b, 0x40_b, 0xc6_b, 0xa3_b, 0x00_b, 0x39_b, 0x27_b});
[[maybe_unused]] constexpr auto dataWithCc = std::to_array<Byte>(
    {0x58_b, 0x15_b, 0xa5_b, 0xa5_b, 0xa5_b, 0xa5_b, 0xa5_b, 0xa5_b, 0xa5_b, 0xa5_b, 0xa5_b, 0xa5_b,
     0xa5_b, 0xa5_b, 0xa5_b, 0xa5_b, 0xab_b, 0xb8_b, 0x1c_b, 0x97_b, 0x1a_b, 0xa7_b, 0x3d_b, 0x3e_b,
     0x77_b, 0x1e_b, 0x34_b, 0x46_b, 0x43_b, 0xed_b, 0xca_b, 0x2c_b, 0xed_b, 0x40_b, 0xbd_b, 0x19_b,
     0x01_b, 0x9c_b, 0xf4_b, 0xf4_b, 0xa7_b, 0x79_b, 0x7c_b, 0xd2_b, 0x1a_b, 0x0c_b, 0x82_b, 0xaf_b,
     0x13_b, 0xfe_b, 0xfd_b, 0x82_b, 0x54_b, 0x17_b, 0xb7_b, 0x94_b, 0x47_b, 0x0f_b, 0x24_b, 0x03_b,
     0x99_b, 0xb8_b, 0x56_b, 0x2a_b, 0x83_b, 0x16_b, 0xf5_b, 0x76_b, 0x86_b, 0x1d_b, 0x7e_b, 0x72_b,
     0xcf_b, 0x74_b, 0xbb_b, 0x29_b, 0xfc_b, 0xc0_b, 0xb6_b, 0xd6_b, 0xa5_b, 0xce_b, 0x36_b, 0x59_b,
     0xe8_b, 0xee_b, 0x9a_b, 0xc7_b, 0x80_b, 0x69_b, 0x23_b, 0x35_b, 0x26_b, 0x3e_b, 0xad_b, 0x3a_b,
     0xe4_b, 0x53_b, 0x21_b, 0x08_b, 0x12_b, 0xbc_b, 0x1f_b, 0x43_b, 0x46_b, 0x4d_b, 0xc6_b, 0xc2_b,
     0x8b_b, 0xe2_b, 0x27_b, 0x7c_b, 0x4f_b, 0xfb_b, 0xf6_b, 0x09_b, 0x50_b, 0x5e_b, 0xde_b, 0x51_b,
     0x1c_b, 0x3c_b, 0x90_b, 0x0e_b, 0x66_b, 0xe1_b, 0x58_b, 0xaa_b, 0x0c_b, 0x5b_b, 0xd5_b, 0xda_b,
     0x18_b, 0x75_b, 0xf9_b, 0xcb_b, 0x3d_b, 0xd2_b, 0xec_b, 0xa7_b, 0xf3_b, 0x02_b, 0xdb_b, 0x5a_b,
     0x97_b, 0x38_b, 0xd9_b, 0x67_b, 0xa3_b, 0xba_b, 0x6b_b, 0x1e_b, 0x01_b, 0xa4_b, 0x8c_b, 0xd4_b,
     0x98_b, 0xfa_b, 0xb4_b, 0xeb_b, 0x91_b, 0x4c_b, 0x84_b, 0x20_b, 0x4a_b, 0xf0_b, 0x7d_b, 0x0d_b,
     0x19_b, 0x37_b, 0x1b_b, 0x0a_b, 0x2f_b, 0x88_b, 0x9d_b, 0xf1_b, 0x3f_b, 0xef_b, 0xd8_b, 0x25_b,
     0x41_b, 0x7b_b, 0x79_b, 0x44_b, 0x70_b, 0xf2_b, 0x40_b, 0x39_b, 0x9b_b, 0x85_b, 0x62_b, 0xa8_b,
     0x31_b, 0x6f_b, 0x57_b, 0x68_b, 0x61_b, 0xd7_b, 0xe7_b, 0x2c_b, 0xf7_b, 0x4b_b, 0xb2_b, 0x9f_b,
     0xcc_b, 0x0b_b, 0x6d_b, 0x6a_b, 0x5c_b, 0xe3_b, 0x65_b, 0x9e_b, 0x8e_b, 0xe9_b, 0xac_b, 0x78_b,
     0x06_b, 0x92_b, 0x33_b, 0x52_b, 0x63_b, 0xea_b, 0xd3_b, 0xae_b, 0x45_b, 0x32_b, 0x10_b, 0x81_b,
     0x2b_b, 0xc1_b, 0xf4_b, 0x34_b, 0x64_b, 0xdc_b, 0x6c_b, 0x28_b, 0xbe_b, 0x22_b, 0x77_b, 0xc4_b,
     0xff_b, 0xbf_b, 0x60_b, 0x95_b, 0x05_b, 0xed_b, 0xe5_b, 0x11_b, 0xc3_b, 0xc9_b, 0x00_b, 0xe6_b,
     0x6e_b, 0x15_b, 0x8a_b, 0xa0_b, 0xc5_b, 0xbd_b, 0x5d_b, 0xa1_b, 0x87_b, 0x5f_b, 0x9c_b, 0xb3_b,
     0xdd_b, 0x2e_b, 0xca_b, 0x7f_b, 0x30_b, 0x2d_b, 0xb5_b, 0xa9_b, 0x73_b, 0x8d_b, 0x96_b, 0x7a_b,
     0x3b_b, 0xa6_b, 0xb1_b, 0xe0_b, 0x1a_b, 0x48_b, 0xcd_b, 0x49_b, 0x8f_b, 0xab_b, 0x4e_b, 0xb9_b,
     0x14_b, 0xc8_b, 0x42_b, 0x04_b, 0xaf_b, 0x07_b, 0xd0_b, 0xd1_b, 0x93_b, 0x71_b, 0xb0_b, 0xa2_b,
     0xf8_b, 0x89_b, 0xdf_b, 0x13_b, 0xfe_b, 0xfd_b, 0x82_b, 0x54_b, 0x17_b, 0xb7_b, 0x94_b, 0x47_b,
     0x0f_b, 0x24_b, 0x03_b, 0x99_b, 0xb8_b, 0x56_b, 0x2a_b, 0x83_b, 0x16_b, 0xf5_b, 0x76_b, 0x86_b,
     0x1d_b, 0x7e_b, 0x72_b, 0xcf_b, 0x74_b, 0xbb_b, 0x29_b, 0xfc_b, 0xc0_b, 0xb6_b, 0xd6_b, 0xa5_b,
     0xce_b, 0x36_b, 0x59_b, 0xe8_b, 0xee_b, 0x9a_b, 0xc7_b, 0x80_b, 0x69_b, 0x23_b, 0x35_b, 0x26_b,
     0x3e_b, 0xad_b, 0x3a_b, 0xe4_b, 0x53_b, 0x21_b, 0x08_b, 0x12_b, 0xbc_b, 0x1f_b, 0x43_b, 0x46_b,
     0x4d_b, 0xc6_b, 0xc2_b, 0x8b_b, 0xe2_b, 0x27_b, 0x7c_b, 0x4f_b, 0xfb_b, 0xf6_b, 0x09_b, 0x50_b,
     0x5e_b, 0xde_b, 0x51_b, 0x1c_b, 0x3c_b, 0x90_b, 0x0e_b, 0x66_b, 0xe1_b, 0x58_b, 0xaa_b, 0x0c_b,
     0x5b_b, 0xd5_b, 0xda_b, 0x18_b, 0x75_b, 0xf9_b, 0xcb_b, 0x3d_b, 0xd2_b, 0xec_b, 0xa7_b, 0xf3_b,
     0x02_b, 0xdb_b, 0x5a_b, 0x97_b, 0x38_b, 0xd9_b, 0x67_b, 0xa3_b, 0xba_b, 0x6b_b, 0x1e_b, 0x01_b,
     0xa4_b, 0x8c_b, 0xd4_b, 0x98_b, 0xfa_b, 0xb4_b, 0xeb_b, 0x91_b, 0x4c_b, 0x84_b, 0x20_b, 0x4a_b,
     0xf0_b, 0x7d_b, 0x0d_b, 0x19_b, 0x37_b, 0x1b_b, 0x0a_b, 0x2f_b, 0x88_b, 0x9d_b, 0xf1_b, 0x3f_b,
     0xef_b, 0xd8_b, 0x25_b, 0x41_b, 0x7b_b, 0x79_b, 0x44_b, 0x70_b, 0xf2_b, 0x40_b, 0x39_b, 0x9b_b,
     0x85_b, 0x62_b, 0xa8_b, 0x31_b, 0x6f_b, 0x57_b, 0x68_b, 0x61_b, 0xd7_b, 0xe7_b, 0x2c_b, 0xf7_b,
     0x4b_b, 0xb2_b, 0x9f_b, 0xcc_b, 0x0b_b, 0x6d_b, 0x6a_b, 0x5c_b, 0xe3_b, 0x65_b, 0x9e_b, 0x8e_b,
     0xe9_b, 0xac_b, 0x75_b, 0xa9_b, 0xa9_b, 0x48_b, 0xa6_b, 0xd7_b, 0xdf_b, 0x5a_b, 0x1e_b, 0x15_b,
     0x50_b, 0x6b_b, 0xe8_b, 0xa9_b, 0x8b_b, 0xe2_b, 0x11_b, 0x2c_b, 0x02_b, 0x85_b, 0x2b_b, 0xaa_b,
     0x51_b, 0x4f_b, 0x03_b, 0x9a_b, 0x3c_b, 0x68_b, 0x8e_b, 0x29_b, 0x5a_b, 0x3c_b, 0x53_b, 0x7f_b,
     0x4c_b, 0x19_b, 0xf4_b, 0xbd_b, 0xa5_b, 0xc3_b, 0x94_b, 0xf5_b, 0x31_b, 0xa1_b, 0x6b_b, 0xfc_b,
     0x84_b, 0xc2_b, 0x2d_b, 0x1e_b, 0x20_b, 0x87_b, 0x9f_b, 0x52_b, 0x81_b, 0xbb_b, 0x82_b, 0x75_b,
     0xd8_b, 0xe5_b, 0x58_b, 0xf4_b, 0x4b_b, 0xe2_b, 0x1f_b, 0xe5_b, 0x00_b, 0x00_b, 0x00_b, 0x00_b,
     0x00_b, 0x00_b, 0x00_b, 0x00_b});

auto led1GpioPin = hal::GpioPin(hal::led1Pin);


class RfSendTest : public RODOS::StaticThread<5'000>
{
public:
    RfSendTest() : StaticThread("RfSendTest")
    {
    }


private:
    void init() override
    {
        led1GpioPin.SetDirection(hal::PinDirection::out);
        led1GpioPin.Reset();
    }


    void run() override
    {
        PRINTF("\nRF test\n\n");

        rf::Initialize(rf::TxType::packet);
        PRINTF("RF module initialized\n");

        auto n = 10U;
        auto message = dataWithoutCc;
        PRINTF("\n");
        PRINTF("Sending a %i bytes long test message %u time(s)\n",
               static_cast<int>(message.size()),
               n);
        DisableRfLatchupProtection();
        for(auto i = 0U; i < n; ++i)
        {
            led1GpioPin.Set();
            auto sendAndWaitResult = rf::SendAndWait(Span(message));
            if(sendAndWaitResult.has_error())
            {
                PRINTF("SendAndWait() %u/%u returned error code %i\n",
                       i,
                       n,
                       static_cast<int>(sendAndWaitResult.error()));
                led1GpioPin.Reset();
                break;
            }
            SuspendFor(100 * ms);
            led1GpioPin.Reset();
            SuspendFor(400 * ms);
        }
        EnableRfLatchupProtection();
        PRINTF("-> done\n");

        auto totalLength = message.size() * n;
        if(totalLength > rf::maxTxDataLength)
        {
            n = rf::maxTxDataLength / message.size();
        }
        PRINTF("\nSending %i %i bytes long message(s) in a single transmission\n\n",
               n,
               static_cast<int>(message.size()));
        rf::SetTxDataLength(static_cast<std::uint16_t>(message.size() * n));
        DisableRfLatchupProtection();
        [&]()
        {
            for(auto i = 0U; i < n; ++i)
            {
                led1GpioPin.Set();
                auto sendAndContinueResult = rf::SendAndContinue(Span(message));
                if(sendAndContinueResult.has_error())
                {
                    PRINTF("SendAndContinue() %u/%u returned error code %i\n",
                           i,
                           n,
                           static_cast<int>(sendAndContinueResult.error()));
                    return;
                }
            }
            auto suspendUntilDataSentResult = rf::SuspendUntilDataSent(1 * s);
            if(suspendUntilDataSentResult.has_error())
            {
                PRINTF("SuspendUntilDataSent() returned error code %i\n",
                       static_cast<int>(suspendUntilDataSentResult.error()));
            }
        }();
        led1GpioPin.Reset();
        rf::EnterStandbyMode();
        EnableRfLatchupProtection();
        PRINTF("-> done\n");
    }
} rfSendTest;
}
