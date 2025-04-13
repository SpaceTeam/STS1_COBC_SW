#include <Sts1CobcSw/CobcSoftware/FirmwareCrcSupervisorThread.hpp>
#include <Sts1CobcSw/CobcSoftware/ThreadPriorities.hpp>
#include <Sts1CobcSw/RodosTime/RodosTime.hpp>
#include <Sts1CobcSw/Utility/Crc32.hpp>
#include <Sts1CobcSw/Utility/DebugPrint.hpp>
#include <Sts1CobcSw/Vocabulary/Time.hpp>

#include <rodos_no_using_namespace.h>

#include <cstdint>
#include <span>

#include "Sts1CobcSw/Utility/Span.hpp"


namespace sts1cobcsw
{
// Running the SpiSupervisor HW test showed that the minimum required stack size is ~560 bytes
constexpr auto stackSize = 600;

constexpr auto firmwareSector1Address = 0x8020000ULL;
constexpr auto firmwareSector2Address = 0x8040000ULL;
constexpr auto firmwareSector3Address = 0x8060000ULL;
constexpr auto maxFirmwareLength = 0x20000ULL;
constexpr auto invalidCrc = 0xFFFFFFFFULL;

// TODO: Think about how often the firmwareCrcSupervisor should run
constexpr auto supervisionPeriod = 1 * h;

class FirmwareCrcSupervisorThread : public RODOS::StaticThread<stackSize>
{
public:
    FirmwareCrcSupervisorThread()
        : StaticThread("FirmwareCrcSupervisorThread", firmwareCrcSupervisorThreadPriority)
    {
    }


private:
    void init() override
    {
    }


    void run() override
    {
        SuspendUntil(endOfTime);
        // TODO: wait until startup checks complete required?
        DEBUG_PRINT("Firmware Crc Supervisor start ...");

        auto const sector1 = CreateSectorSpan(firmwareSector1Address);
        auto const sector2 = CreateSectorSpan(firmwareSector2Address);
        auto const sector3 = CreateSectorSpan(firmwareSector3Address);

        while(true)
        {
            if(CrcSector(sector1) != 0)
            {
                DEBUG_PRINT("Firmware Sector 1 damaged performing a reboot ...");
                RODOS::hwResetAndReboot();
            }
            if(CrcSector(sector2) != 0)
            {
                DEBUG_PRINT("Firmware Sector 2 damaged performing a reboot ...");
                RODOS::hwResetAndReboot();
            }
            if(CrcSector(sector3) != 0)
            {
                DEBUG_PRINT("Firmware Sector 3 damaged performing a reboot ...");
                RODOS::hwResetAndReboot();
            }

            SuspendFor(supervisionPeriod);
        }
    }


    static auto CreateSectorSpan(std::uint32_t startAddress) -> std::span<Byte const>
    {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast, performance-no-int-to-ptr)
        auto * startPointer = reinterpret_cast<void *>(startAddress);
        auto length = 0U;
        std::memcpy(&length, startPointer, sizeof(length));
        if(length > maxFirmwareLength)
        {
            return {};
        }
        // Add last 4 bytes for existing crc32
        return {static_cast<Byte const *>(startPointer), length + 4};
    }


    static auto CrcSector(std::span<Byte const> sector) -> std::uint32_t
    {
        // Using same crc logic and initial value as the bootloader uses
        if(sector.empty())
        {
            return invalidCrc;
        }
        return utility::ComputeCrc32(0x00U, sector);
    }
} firmwareCrcSupervisorThread;


auto ResumeFirmwareCrcSupervisorThread() -> void
{
    firmwareCrcSupervisorThread.resume();
}
}
