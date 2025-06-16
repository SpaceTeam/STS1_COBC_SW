#include <Sts1CobcSw/Firmware/RfCommunicationThread.hpp>
#include <Sts1CobcSw/Firmware/SpiStartupTestAndSupervisorThread.hpp>
#include <Sts1CobcSw/Firmware/ThreadPriorities.hpp>
#include <Sts1CobcSw/Firmware/TopicsAndSubscribers.hpp>
#include <Sts1CobcSw/Fram/Fram.hpp>
#include <Sts1CobcSw/FramSections/FramLayout.hpp>
#include <Sts1CobcSw/FramSections/FramRingArray.hpp>
#include <Sts1CobcSw/FramSections/PersistentVariables.hpp>
#include <Sts1CobcSw/Mailbox/Mailbox.hpp>
#include <Sts1CobcSw/RealTime/RealTime.hpp>
#include <Sts1CobcSw/RodosTime/RodosTime.hpp>
#include <Sts1CobcSw/Sensors/Eps.hpp>
#include <Sts1CobcSw/Sensors/TemperatureSensor.hpp>
#include <Sts1CobcSw/Serial/UInt.hpp>
#include <Sts1CobcSw/Telemetry/TelemetryMemory.hpp>
#include <Sts1CobcSw/Telemetry/TelemetryRecord.hpp>
#include <Sts1CobcSw/Utility/ErrorDetectionAndCorrection.hpp>
#include <Sts1CobcSw/Vocabulary/MessageTypeIdFields.hpp>
#include <Sts1CobcSw/Vocabulary/ProgramId.hpp>
#include <Sts1CobcSw/Vocabulary/Time.hpp>

#include <strong_type/affine_point.hpp>
#include <strong_type/difference.hpp>
#include <strong_type/type.hpp>

#include <rodos_no_using_namespace.h>

#include <cstdint>
#include <utility>


namespace sts1cobcsw
{
namespace
{
constexpr auto stackSize = 4000U;
constexpr auto telemetryThreadInterval = 30 * s;


[[nodiscard]] auto CollectTelemetryData() -> TelemetryRecord;


class TelemetryThread : public RODOS::StaticThread<stackSize>
{
public:
    TelemetryThread() : StaticThread("TelemetryThread", telemetryThreadPriority)
    {}


private:
    void init() override
    {}


    void run() override
    {
        SuspendFor(totalStartupTestTimeout);  // Wait for the startup tests to complete
        TIME_LOOP(0, value_of(telemetryThreadInterval))
        {
            persistentVariables.Store<"realTime">(CurrentRealTime());
            auto telemetryRecord = CollectTelemetryData();
            telemetryMemory.PushBack(telemetryRecord);
            telemetryRecordMailbox.Overwrite(telemetryRecord);
            ResumeRfCommunicationThread();
        }
    }
} telemetryThread;


auto CollectTelemetryData() -> TelemetryRecord
{
    auto eduIsAlive = false;
    eduIsAliveBufferForTelemetry.get(eduIsAlive);
    auto programIdOfCurrentEduProgramQueueEntry = ProgramId(0);
    programIdOfCurrentEduProgramQueueEntryBuffer.get(programIdOfCurrentEduProgramQueueEntry);
    std::int32_t rxBaudRate = 0;
    rxBaudRateBuffer.get(rxBaudRate);
    std::int32_t txBaudRate = 0;
    txBaudRateBuffer.get(txBaudRate);
    return TelemetryRecord{
        // Booleans: byte 1: EDU and housekeeping
        .eduShouldBePowered = persistentVariables.Load<"eduShouldBePowered">() ? 1 : 0,
        .eduIsAlive = eduIsAlive ? 1 : 0,
        .newEduResultIsAvailable = persistentVariables.Load<"newEduResultIsAvailable">() ? 1 : 0,
        .antennasShouldBeDeployed = persistentVariables.Load<"antennasShouldBeDeployed">() ? 1 : 0,
        .framIsWorking = fram::framIsWorking.Load() ? 1 : 0,
        .epsIsWorking = persistentVariables.Load<"epsIsWorking">() ? 1 : 0,
        .flashIsWorking = persistentVariables.Load<"flashIsWorking">() ? 1 : 0,
        .rfIsWorking = persistentVariables.Load<"rfIsWorking">() ? 1 : 0,
        // Booleans: byte 2:  and communication
        .lastMessageTypeIdWasInvalid =
            persistentVariables.Load<"lastMessageTypeIdWasInvalid">() ? 1 : 0,
        .lastApplicationDataWasInvalid =
            persistentVariables.Load<"lastApplicationDataWasInvalid">() ? 1 : 0,
        // BootLoader
        .nTotalResets = persistentVariables.Load<"nTotalResets">(),
        .nResetsSinceRf = persistentVariables.Load<"nResetsSinceRf">(),
        .activeSecondaryFwPartition = persistentVariables.Load<"activeSecondaryFwPartition">(),
        .backupSecondaryFwPartition = persistentVariables.Load<"backupSecondaryFwPartition">(),
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
        .rxBaudRate = rxBaudRate,
        .txBaudRate = txBaudRate,
        .nCorrectableUplinkErrors = persistentVariables.Load<"nCorrectableUplinkErrors">(),
        .nUncorrectableUplinkErrors = persistentVariables.Load<"nUncorrectableUplinkErrors">(),
        .nGoodTransferFrames = persistentVariables.Load<"nGoodTransferFrames">(),
        .nBadTransferFrames = persistentVariables.Load<"nBadTransferFrames">(),
        .lastFrameSequenceNumber = persistentVariables.Load<"lastFrameSequenceNumber">(),
        .lastMessageTypeId = persistentVariables.Load<"lastMessageTypeId">()};
}
}
}
