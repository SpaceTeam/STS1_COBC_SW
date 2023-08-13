//! @file RfSimple.test.cpp
//! @brief A program to test some basic functionality of the Si4463 RF chip.
//!
//! This test does not perform any transmissions, it simply powers up the chip and tries to get the
//! part info.
//!
//! Prerequisites:
//!     - Working UART to see the test output
//! Preparation:
//!     - Connect the UCI UART to a computer to use with HTERM, Putty, etc.
//!     - Flash the program
//!
//! Now you can check the whether the part info is correct and thus also infer whether the chip was
//! successfully powered on.

#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/Periphery/Rf.hpp>

#include <rodos_no_using_namespace.h>


namespace sts1cobcsw
{

// auto rfSpi = RODOS::HAL_SPI(
//     hal::rfSpiIndex, hal::rfSpiSckPin, hal::rfSpiMisoPin, hal::rfSpiMosiPin,
//     hal::spiNssDummyPin);


class RfSimpleTest : public RODOS::StaticThread<>
{
public:
    RfSimpleTest() : StaticThread("RfSimpleTest", 50)
    {
    }

private:
    void init() override
    {
        RODOS::PRINTF("Hello RfSimple\n");
    }


    void run() override
    {
        periphery::rf::InitializeGpioAndSpi();
        constexpr auto bootOptions = periphery::rf::PowerUpBootOptions::noPatch;
        constexpr auto xtalOptions = periphery::rf::PowerUpXtalOptions::xtal;
        constexpr std::uint32_t xoFrequency = 30'000'000;
        periphery::rf::PowerUp(bootOptions, xtalOptions, xoFrequency);
        RODOS::PRINTF("RfSimple Test: initialized and powered up\n");
        RODOS::PRINTF("Try reading part info...\n");

        // Program seems to stop/crash when waiting on CTS ready byte.
        // SPI3 seems to work in general, as it is possible to measure some test output.

        auto receivedPartInfo = periphery::rf::GetPartInfo();
        RODOS::PRINTF("Received part info: %x\n", receivedPartInfo);
    }

} rfSimpleTest;
}