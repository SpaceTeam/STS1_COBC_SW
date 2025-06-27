#include <Sts1CobcSw/Bootloader/Leds.hpp>
#include <Sts1CobcSw/Bootloader/RunFirmware.hpp>
#include <Sts1CobcSw/Bootloader/UciUart.hpp>
#include <Sts1CobcSw/Bootloader/UciSpi.hpp>

namespace
{
namespace persvar
{
constexpr auto persistentVariableBlockSize = 100; //There are 3 blocks together
constexpr auto nResetsSinceRfAdress = 0;
constexpr auto activeSecondaryFwPartitionAdress = 1;   //O oder 255
constexpr auto backupSecondaryFwPartitionAdress = 2;   //O oder 255
constexpr auto nTotalResetsAdress = 6; //TODO: check this //Together 4 bytes 
}
}

auto main() -> int
{
    sts1cobcsw::leds::Initialize();
    sts1cobcsw::leds::TurnOn();
    sts1cobcsw::uciuart::Initialize();
    sts1cobcsw::uciuart::Write("Hello from the bootloader!\n");
    
    sts1cobcsw::ucispi::Initialize();
    char nResetsSinceRf = 0;
    sts1cobcsw::ucispi::FramRead(persvar::nResetsSinceRfAdress, &nResetsSinceRf, 1);
    sts1cobcsw::uciuart::Write("Number of resets since Rf: \n");
    sts1cobcsw::uciuart::Write(nResetsSinceRf);
    nResetsSinceRf = static_cast<char>(static_cast<int>(nResetsSinceRf)+1);
    sts1cobcsw::ucispi::FramWrite(persvar::nResetsSinceRfAdress, &nResetsSinceRf, 1);
    
    //sts1cobcsw::RunFirmware();
}
