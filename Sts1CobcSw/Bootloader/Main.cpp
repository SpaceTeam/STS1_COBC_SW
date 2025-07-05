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
    
    //increment the number of total resets
    //read the variable from the fram
    unsigned int byte[4] = {0x00};
    byte[0] = sts1cobcsw::bootloader::fram::PersistentWariableRead(persvar::nTotalResetsAdress,
                                                                   persvar::persistentVariableBlockSize);
    byte[1] = sts1cobcsw::bootloader::fram::PersistentWariableRead(persvar::nTotalResetsAdress + 1,
                                                                   persvar::persistentVariableBlockSize);
    byte[2] = sts1cobcsw::bootloader::fram::PersistentWariableRead(persvar::nTotalResetsAdress + 2,
                                                                   persvar::persistentVariableBlockSize);
    byte[3] = sts1cobcsw::bootloader::fram::PersistentWariableRead(persvar::nTotalResetsAdress + 3,
                                                                   persvar::persistentVariableBlockSize);
    unsigned long value = byte[0]*(256*256*256) + byte[1]*(256*256) + byte[2]*256 + byte[3];
    //write the incremented value back to the fram
    value++;
    byte[0] = (value & 0xFF000000)>>24;
    byte[1] = (value & 0x00FF0000)>>16;
    byte[2] = (value & 0x0000FF00)>>8;
    byte[3] = (value & 0x000000FF);
    sts1cobcsw::bootloader::fram::PersistentWariableWrite(persvar::nTotalResetsAdress,
                                                          byte[0],
                                                          persvar::persistentVariableBlockSize);
    sts1cobcsw::bootloader::fram::PersistentWariableWrite(persvar::nTotalResetsAdress+1,
                                                          byte[1],
                                                          persvar::persistentVariableBlockSize);
    sts1cobcsw::bootloader::fram::PersistentWariableWrite(persvar::nTotalResetsAdress+2,
                                                          byte[2],
                                                          persvar::persistentVariableBlockSize);
    sts1cobcsw::bootloader::fram::PersistentWariableWrite(persvar::nTotalResetsAdress+3,
                                                          byte[3],
                                                          persvar::persistentVariableBlockSize);
  
    //increment the number of resets since the last Rf
    auto nResetsSinceRf = sts1cobcsw::bootloader::fram::PersistentWariableRead(persvar::nResetsSinceRfAdress, 
                                                         persvar::persistentVariableBlockSize);
    sts1cobcsw::bootloader::fram::PersistentWariableWrite(persvar::nResetsSinceRfAdress, 
                                                          nResetsSinceRf+1, 
                                                          persvar::persistentVariableBlockSize);
    
    sts1cobcsw::bootloader::uciuart::Write("Number of resets since Rf: ");
    sts1cobcsw::bootloader::utilities::PrintDecString(reinterpret_cast<const char*>(&nResetsSinceRf), 1);
    sts1cobcsw::bootloader::uciuart::Write("\n");
    
    value--;
    sts1cobcsw::bootloader::uciuart::Write("Number of total resets: ");
    sts1cobcsw::bootloader::utilities::PrintDecString(reinterpret_cast<const char*>(&value), 4);
    sts1cobcsw::bootloader::uciuart::Write("\n");
    
    sts1cobcsw::bootloader::RunFirmware();
}
