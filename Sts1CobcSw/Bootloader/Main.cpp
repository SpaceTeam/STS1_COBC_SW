#include <Sts1CobcSw/Bootloader/DebugPrint.hpp>
#include <Sts1CobcSw/Bootloader/Fram.hpp>
#include <Sts1CobcSw/Bootloader/PersistentVariables.hpp>
#include <Sts1CobcSw/Bootloader/RunFirmware.hpp>
#ifdef ENABLE_DEBUG_PRINT
    #include <Sts1CobcSw/Bootloader/Leds.hpp>
    #include <Sts1CobcSw/Bootloader/UciUart.hpp>
#endif
#include <Sts1CobcSw/FirmwareManagement/FirmwareManagement.hpp>
#include <Sts1CobcSw/Outcome/Outcome.hpp>
#include <Sts1CobcSw/Vocabulary/Ids.hpp>


namespace fw = sts1cobcsw::fw;
using sts1cobcsw::ErrorCode;


namespace
{
constexpr int nResetsSinceRfLimit = 50;


auto GetPrimaryFirmwareChecksums() -> fw::FirmwareChecksums;
auto GetSecondaryFirmwareChecksums(fw::Partition const & secondaryPartition)
    -> fw::FirmwareChecksums;
}


auto main() -> int
{
#ifdef ENABLE_DEBUG_PRINT
    sts1cobcsw::leds::Initialize();
    sts1cobcsw::leds::TurnOn();
    sts1cobcsw::uciuart::Initialize();
#endif
    sts1cobcsw::fram::Initialize();
    DEBUG_PRINT("\n");
    DEBUG_PRINT("Hello from the bootloader!\n");
    DEBUG_PRINT("\n");

    // Load and increment the reset counters
    auto nTotalResets = sts1cobcsw::Load(sts1cobcsw::nTotalResets);
    auto nResetsSinceRf = sts1cobcsw::Load(sts1cobcsw::nResetsSinceRf);
    ++nTotalResets;
    ++nResetsSinceRf;
    DEBUG_PRINT("nTotalResets   = %d\n", static_cast<int>(nTotalResets));
    DEBUG_PRINT("nResetsSinceRf = %d\n", static_cast<int>(nResetsSinceRf));
    DEBUG_PRINT("\n");
    sts1cobcsw::Store(sts1cobcsw::nTotalResets, nTotalResets);
    sts1cobcsw::Store(sts1cobcsw::nResetsSinceRf, nResetsSinceRf);

    // Load the active and backup secondary FW partition IDs
    auto backupSecondaryFwPartitionId = sts1cobcsw::Load(sts1cobcsw::backupSecondaryFwPartitionId);
    DEBUG_PRINT("backupSecondaryFwPartitionId = 0x%02X\n",
                static_cast<unsigned>(backupSecondaryFwPartitionId));
    auto activeSecondaryFwPartitionId = sts1cobcsw::Load(sts1cobcsw::activeSecondaryFwPartitionId);
    DEBUG_PRINT("activeSecondaryFwPartitionId = 0x%02X\n",
                static_cast<unsigned>(activeSecondaryFwPartitionId));
    DEBUG_PRINT("\n");

    if(nResetsSinceRf > nResetsSinceRfLimit)
    {
        DEBUG_PRINT("Reset limit reached, using backup FW\n");
        DEBUG_PRINT("\n");
        activeSecondaryFwPartitionId = backupSecondaryFwPartitionId;
        sts1cobcsw::Store(sts1cobcsw::activeSecondaryFwPartitionId, activeSecondaryFwPartitionId);
    }

    auto primaryFirmwareChecksums = GetPrimaryFirmwareChecksums();
    auto secondaryPartition = fw::GetPartition(activeSecondaryFwPartitionId);
    auto secondaryFirmwareChecksums = GetSecondaryFirmwareChecksums(secondaryPartition);
    DEBUG_PRINT("Primary FW checksums:   stored   = 0x%08X\n",
                static_cast<unsigned>(primaryFirmwareChecksums.stored));
    DEBUG_PRINT("                        computed = 0x%08X\n",
                static_cast<unsigned>(primaryFirmwareChecksums.computed));
    DEBUG_PRINT("Secondary FW checksums: stored   = 0x%08X\n",
                static_cast<unsigned>(secondaryFirmwareChecksums.stored));
    DEBUG_PRINT("                        computed = 0x%08X\n",
                static_cast<unsigned>(secondaryFirmwareChecksums.computed));
    DEBUG_PRINT("\n");

    if(secondaryFirmwareChecksums.computed == secondaryFirmwareChecksums.stored)
    {
        if(primaryFirmwareChecksums.computed != secondaryFirmwareChecksums.computed)
        {
            DEBUG_PRINT("Primary FW != secondary FW -> overwriting primary with secondary FW\n");
            auto overwriteSucceeded = fw::Overwrite(fw::DestinationPartition(fw::primaryPartition),
                                                    fw::SourcePartition(secondaryPartition));
            if(not overwriteSucceeded)
            {
                DEBUG_PRINT("  -> failed\n");
                // TODO: What do we do here?
            }
            DEBUG_PRINT("\n");
        }
    }
    else
    {
        if(primaryFirmwareChecksums.computed == primaryFirmwareChecksums.stored)
        {
            DEBUG_PRINT("Secondary FW corrupt -> overwriting with primary one\n");
            auto overwriteSucceeded = fw::Overwrite(fw::DestinationPartition(secondaryPartition),
                                                    fw::SourcePartition(fw::primaryPartition));
            if(not overwriteSucceeded)
            {
                DEBUG_PRINT("  -> failed\n");
                // TODO: What do we do here?
            }
            DEBUG_PRINT("\n");
        }
    }

    sts1cobcsw::RunFirmware();
}


namespace
{
auto GetPrimaryFirmwareChecksums() -> fw::FirmwareChecksums
{
    auto errorCode = ErrorCode::eduIsNotAlive;
    auto primaryFirmwareChecksums =
        fw::ComputeAndReadFirmwareChecksums(fw::primaryPartition.startAddress, &errorCode);
    if(errorCode != ErrorCode::eduIsNotAlive)
    {
        DEBUG_PRINT("Failed to compute and read primary partition checksums: %s\n",
                    sts1cobcsw::ToCZString(errorCode));
        // TODO: What do we do here?
    }
    return primaryFirmwareChecksums;
}


auto GetSecondaryFirmwareChecksums(fw::Partition const & secondaryPartition)
    -> fw::FirmwareChecksums
{
    auto errorCode = ErrorCode::eduIsNotAlive;
    auto secondaryFirmwareChecksums =
        fw::ComputeAndReadFirmwareChecksums(secondaryPartition.startAddress, &errorCode);
    if(errorCode != ErrorCode::eduIsNotAlive)
    {
        DEBUG_PRINT("Failed to compute and read secondary partition checksums: %s\n",
                    sts1cobcsw::ToCZString(errorCode));
        // TODO: What do we do here?
    }
    return secondaryFirmwareChecksums;
}
}
