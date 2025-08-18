#include <Sts1CobcSw/Bootloader/Fram.hpp>
#include <Sts1CobcSw/Bootloader/PersistentVariables.hpp>
#include <Sts1CobcSw/Bootloader/Print.hpp>
#include <Sts1CobcSw/Bootloader/RunFirmware.hpp>
#ifdef ENABLE_DEBUG_PRINT
    #include <Sts1CobcSw/Bootloader/Leds.hpp>
    #include <Sts1CobcSw/Bootloader/UciUart.hpp>
#endif


auto main() -> int
{
#ifdef ENABLE_DEBUG_PRINT
    sts1cobcsw::leds::Initialize();
    sts1cobcsw::leds::TurnOn();
    sts1cobcsw::uciuart::Initialize();
#endif
    sts1cobcsw::fram::Initialize();
    DEBUG_PRINT("Hello from the bootloader!\n");

    auto nTotalResets = sts1cobcsw::Load(sts1cobcsw::nTotalResets);
    auto nResetsSinceRf = sts1cobcsw::Load(sts1cobcsw::nResetsSinceRf);
    auto activeSecondaryFwPartition = sts1cobcsw::Load(sts1cobcsw::activeSecondaryFwPartition);
    auto backupSecondaryFwPartition = sts1cobcsw::Load(sts1cobcsw::backupSecondaryFwPartition);
    DEBUG_PRINT("nTotalResets               = %d\n", static_cast<int>(nTotalResets));
    DEBUG_PRINT("nResetsSinceRf             = %d\n", static_cast<int>(nResetsSinceRf));
    DEBUG_PRINT("activeSecondaryFwPartition = %02x\n",
                static_cast<unsigned>(activeSecondaryFwPartition));
    DEBUG_PRINT("backupSecondaryFwPartition = %02x\n",
                static_cast<unsigned>(backupSecondaryFwPartition));

    nTotalResets++;
    nResetsSinceRf++;
    sts1cobcsw::Store(sts1cobcsw::nTotalResets, nTotalResets);
    sts1cobcsw::Store(sts1cobcsw::nResetsSinceRf, nResetsSinceRf);

    sts1cobcsw::RunFirmware();
}
