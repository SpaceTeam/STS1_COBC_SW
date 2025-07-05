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
constexpr auto persistentVariableBlockSize = static_cast<unsigned long>(100);  // There are 3 blocks together
constexpr auto nResetsSinceRfAdress = static_cast<unsigned long>(1);
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
    
    auto nResetsSinceRf = sts1cobcsw::bootloader::fram::PersistentWariableRead(persvar::nResetsSinceRfAdress, 
                                                         persvar::persistentVariableBlockSize);
    sts1cobcsw::bootloader::fram::PersistentWariableWrite(persvar::nResetsSinceRfAdress, 
                                                          nResetsSinceRf+1, 
                                                          persvar::persistentVariableBlockSize);
    
    sts1cobcsw::bootloader::uciuart::Write("Number of resets since Rf: ");
    sts1cobcsw::bootloader::utilities::PrintHexString(reinterpret_cast<const char*>(&nResetsSinceRf), 1);
    sts1cobcsw::bootloader::uciuart::Write("\n");
    
    sts1cobcsw::bootloader::RunFirmware();
}
