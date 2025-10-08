#include <Sts1CobcSw/Edu/Edu.hpp>
#include <Sts1CobcSw/ErrorDetectionAndCorrection/EdacVariable.hpp>
#include <Sts1CobcSw/Firmware/RfCommunicationThread.hpp>
#include <Sts1CobcSw/Firmware/StartupAndSpiSupervisorThread.hpp>
#include <Sts1CobcSw/Firmware/ThreadPriorities.hpp>
#include <Sts1CobcSw/Firmware/TopicsAndSubscribers.hpp>
#include <Sts1CobcSw/FirmwareManagement/FirmwareManagement.hpp>  // IWYU pragma: keep
#include <Sts1CobcSw/Fram/Fram.hpp>
#include <Sts1CobcSw/FramSections/FramLayout.hpp>
#include <Sts1CobcSw/FramSections/FramRingArray.hpp>
#include <Sts1CobcSw/FramSections/PersistentVariables.hpp>
#include <Sts1CobcSw/Hal/GpioPin.hpp>
#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/Mailbox/Mailbox.hpp>
#include <Sts1CobcSw/Outcome/Outcome.hpp>  // IWYU pragma: keep
#include <Sts1CobcSw/RealTime/RealTime.hpp>
#include <Sts1CobcSw/RodosTime/RodosTime.hpp>
#include <Sts1CobcSw/Sensors/Eps.hpp>
#include <Sts1CobcSw/Sensors/TemperatureSensor.hpp>
#include <Sts1CobcSw/Serial/UInt.hpp>
#include <Sts1CobcSw/Telemetry/TelemetryMemory.hpp>
#include <Sts1CobcSw/Telemetry/TelemetryRecord.hpp>
#include <Sts1CobcSw/Utility/DebugPrint.hpp>
#include <Sts1CobcSw/Vocabulary/Ids.hpp>
#include <Sts1CobcSw/Vocabulary/MessageTypeIdFields.hpp>
#include <Sts1CobcSw/Vocabulary/Time.hpp>

#include <strong_type/affine_point.hpp>
#include <strong_type/difference.hpp>
#include <strong_type/ordered.hpp>
#include <strong_type/type.hpp>

#include <rodos_no_using_namespace.h>

#include <compare>
#include <cstdint>
#include <utility>


namespace sts1cobcsw
{
namespace
{
constexpr auto stackSize = 1200U;
constexpr auto telemetryThreadInterval = 30 * s;
// The delay until the first telemetry record is published and thereby the first beacon is sent.
// This prevents immediate resets if we boot with too little power for the RF module.
constexpr auto initialTelemetryDelay = 1 * min;
// The time after which we enable resets in case of firmware corruption. This prevents immediate
// resets after booting into a corrupt firmware.
constexpr auto firmwareCorruptionResetEnableTime = RodosTime(0) + 10 * min;

auto epsFaultGpioPin = hal::GpioPin(hal::epsFaultPin);
auto epsChargingGpioPin = hal::GpioPin(hal::epsChargingPin);


[[nodiscard]] auto CheckFirmwareIntegrities() -> bool;
[[nodiscard]] auto CollectTelemetryData() -> TelemetryRecord;


class TelemetryThread : public RODOS::StaticThread<stackSize>
{
public:
    TelemetryThread() : StaticThread("TelemetryThread", telemetryThreadPriority)
    {}


private:
    void init() override
    {
        epsFaultGpioPin.SetDirection(hal::PinDirection::in);
        epsFaultGpioPin.ActivatePullUp();
        epsChargingGpioPin.SetDirection(hal::PinDirection::in);
        epsChargingGpioPin.ActivatePullUp();
    }


    void run() override
    {
        SuspendFor(totalStartupTestTimeout);  // Wait for the startup tests to complete
        DEBUG_PRINT("Starting telemetry thread\n");
        SuspendFor(initialTelemetryDelay);
        TIME_LOOP(0, value_of(telemetryThreadInterval))
        {
            auto firmwareIsIntact = CheckFirmwareIntegrities();
            persistentVariables.Store<"realTime">(CurrentRealTime());
            auto telemetryRecord = CollectTelemetryData();
            telemetryMemory.PushBack(telemetryRecord);
            DEBUG_PRINT("Publishing telemetry record\n");
            telemetryRecordMailbox.Overwrite(telemetryRecord);
            nextTelemetryRecordTimeMailbox.Overwrite(CurrentRodosTime() + telemetryThreadInterval);
            ResumeRfCommunicationThread();
            DEBUG_PRINT_STACK_USAGE();
            if(not firmwareIsIntact and CurrentRodosTime() > firmwareCorruptionResetEnableTime)
            {
                // Wait with the reset long enough to ensure that the telemetry record can be sent
                static constexpr auto resetDelay = 5 * s;
                DEBUG_PRINT("Firmware integrity check failed -> resetting in %d seconds\n",
                            static_cast<int>(resetDelay / s));
                SuspendFor(resetDelay);
                RODOS::hwResetAndReboot();
            }
        }
    }
} telemetryThread;


auto CheckFirmwareIntegrities() -> bool
{
#ifdef BUILD_FOR_USE_WITH_BOOTLOADER
    auto primaryResult = fw::CheckFirmwareIntegrity(fw::primaryPartition.startAddress);
    if(primaryResult.has_error())
    {
        DEBUG_PRINT("Failed firmware integrity check for partition %s: %s\n",
                    ToCZString(PartitionId::primary),
                    ToCZString(primaryResult.error()));
        persistentVariables.Increment<"nFirmwareChecksumErrors">();
        return false;
    }
    auto const secondaryPartitionId = persistentVariables.Load<"activeSecondaryFwPartitionId">();
    auto const secondaryResult =
        fw::CheckFirmwareIntegrity(fw::GetPartition(secondaryPartitionId).startAddress);
    if(secondaryResult.has_error())
    {
        DEBUG_PRINT("Failed firmware integrity check for partition %s: %s\n",
                    ToCZString(secondaryPartitionId),
                    ToCZString(secondaryResult.error()));
        persistentVariables.Increment<"nFirmwareChecksumErrors">();
        return false;
    }
    return true;
#else
    return true;
#endif
}


auto CollectTelemetryData() -> TelemetryRecord
{
    auto eduIsAlive = false;
    eduIsAliveBufferForTelemetry.get(eduIsAlive);
    auto programIdOfCurrentEduProgramQueueEntry = ProgramId(0);
    programIdOfCurrentEduProgramQueueEntryBuffer.get(programIdOfCurrentEduProgramQueueEntry);
    std::uint32_t rxDataRate = 0;
    rxDataRateBuffer.get(rxDataRate);
    std::uint32_t txDataRate = 0;
    txDataRateBuffer.get(txDataRate);
    return TelemetryRecord{
        // Booleans: byte 1
        .eduShouldBePowered = persistentVariables.Load<"eduShouldBePowered">() ? 1 : 0,
        .eduIsAlive = eduIsAlive ? 1 : 0,
        .newEduResultIsAvailable = persistentVariables.Load<"newEduResultIsAvailable">() ? 1 : 0,
        .dosimeterIsPowered = (edu::dosiEnableGpioPin.Read() == hal::PinState::set) ? 1 : 0,
        .antennasShouldBeDeployed = persistentVariables.Load<"antennasShouldBeDeployed">() ? 1 : 0,
        .epsIsCharging = (epsChargingGpioPin.Read() == hal::PinState::set) ? 1 : 0,
        .epsDetectedFault = (epsFaultGpioPin.Read() == hal::PinState::set) ? 1 : 0,
        .framIsWorking = fram::framIsWorking.Load() ? 1 : 0,
        // Booleans: byte 2
        .epsIsWorking = persistentVariables.Load<"epsIsWorking">() ? 1 : 0,
        .flashIsWorking = persistentVariables.Load<"flashIsWorking">() ? 1 : 0,
        .rfIsWorking = persistentVariables.Load<"rfIsWorking">() ? 1 : 0,
        .lastMessageTypeIdWasInvalid =
            persistentVariables.Load<"lastMessageTypeIdWasInvalid">() ? 1 : 0,
        .lastApplicationDataWasInvalid =
            persistentVariables.Load<"lastApplicationDataWasInvalid">() ? 1 : 0,
        // BootLoader
        .nTotalResets = persistentVariables.Load<"nTotalResets">(),
        .nResetsSinceRf = persistentVariables.Load<"nResetsSinceRf">(),
        .activeSecondaryFwPartitionId = persistentVariables.Load<"activeSecondaryFwPartitionId">(),
        .backupSecondaryFwPartitionId = persistentVariables.Load<"backupSecondaryFwPartitionId">(),
        // EDU
        .eduProgramQueueIndex = persistentVariables.Load<"eduProgramQueueIndex">(),
        .programIdOfCurrentEduProgramQueueEntry = programIdOfCurrentEduProgramQueueEntry,
        .nEduCommunicationErrors = persistentVariables.Load<"nEduCommunicationErrors">(),
        // Housekeeping
        .lastResetReason = 0U,  // TODO: Get with RCC_GetFlagStatus() (needs to be called in main)
        .rodosTimeInSeconds = static_cast<std::int32_t>((CurrentRodosTime() - RodosTime{}) / s),
        .realTime = CurrentRealTime(),
        .nFirmwareChecksumErrors = persistentVariables.Load<"nFirmwareChecksumErrors">(),
        .nFlashErrors = persistentVariables.Load<"nFlashErrors">(),
        .nRfErrors = persistentVariables.Load<"nRfErrors">(),
        .nFileSystemErrors = persistentVariables.Load<"nFileSystemErrors">(),
        // Sensor data
        .cobcTemperature = 0U,  // TODO: Get from internal ADC
        .rfTemperature = rftemperaturesensor::Read(),
        .epsAdcData = eps::ReadAdcs(),
        // Communication
        .rxDataRate = rxDataRate,
        .txDataRate = txDataRate,
        .nCorrectableUplinkErrors = persistentVariables.Load<"nCorrectableUplinkErrors">(),
        .nUncorrectableUplinkErrors = persistentVariables.Load<"nUncorrectableUplinkErrors">(),
        .nGoodTransferFrames = persistentVariables.Load<"nGoodTransferFrames">(),
        .nBadTransferFrames = persistentVariables.Load<"nBadTransferFrames">(),
        .lastFrameSequenceNumber = persistentVariables.Load<"lastFrameSequenceNumber">(),
        .lastMessageTypeId = persistentVariables.Load<"lastMessageTypeId">(),
        .fileTransferStatus = fileTransferStatus.Load(),
        .transactionSequenceNumber = transactionSequenceNumber.Load()};
}
}
}
