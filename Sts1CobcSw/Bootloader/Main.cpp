#include <Sts1CobcSw/Bootloader/Fram.hpp>
#include <Sts1CobcSw/Bootloader/Leds.hpp>
#include <Sts1CobcSw/Bootloader/RunFirmware.hpp>
#include <Sts1CobcSw/Bootloader/Spi.hpp>
#include <Sts1CobcSw/Bootloader/UciUart.hpp>
#include <Sts1CobcSw/Bootloader/Utilities.hpp>

namespace
{
namespace persvar
{
constexpr auto persistentVariableBlockSize = 100;  // There are 3 blocks together
constexpr unsigned long nResetsSinceRfAdress = 1;
constexpr auto activeSecondaryFwPartitionAdress = 1;  // O or 255
constexpr auto backupSecondaryFwPartitionAdress = 2;  // O or 255
constexpr auto nTotalResetsAdress = 6;                // TODO: check this //Together 4 bytes
}
}

auto main() -> int
{
    sts1cobcsw::bootloader::leds::Initialize();
    sts1cobcsw::bootloader::leds::TurnOn();
    sts1cobcsw::bootloader::uciuart::Initialize();
    sts1cobcsw::bootloader::uciuart::Write("Hello from the bootloader!\n");

    sts1cobcsw::bootloader::fram::Initialize();

    sts1cobcsw::bootloader::fram::ReadId();
    
    char *nResetsSinceRf = new char;
    sts1cobcsw::bootloader::fram::Read(persvar::nResetsSinceRfAdress, &nResetsSinceRf, 1);
    
    sts1cobcsw::bootloader::uciuart::Write("Number of resets since Rf: ");
    sts1cobcsw::bootloader::utilities::PrintHexString(nResetsSinceRf, 1);
    sts1cobcsw::bootloader::uciuart::Write("\n");
    
    *nResetsSinceRf = static_cast<char>(static_cast<int>(*nResetsSinceRf) + 1);
    sts1cobcsw::bootloader::fram::Write(persvar::nResetsSinceRfAdress, nResetsSinceRf, 1);

    sts1cobcsw::bootloader::RunFirmware();
}
