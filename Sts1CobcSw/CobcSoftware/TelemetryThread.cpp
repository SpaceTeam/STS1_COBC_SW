#include <Sts1CobcSw/CobcSoftware/RfCommunicationThread.hpp>
#include <Sts1CobcSw/CobcSoftware/ThreadPriorities.hpp>
#include <Sts1CobcSw/CobcSoftware/TopicsAndSubscribers.hpp>
#include <Sts1CobcSw/FramSections/FramLayout.hpp>
#include <Sts1CobcSw/FramSections/FramRingArray.hpp>
#include <Sts1CobcSw/FramSections/PersistentVariables.hpp>
#include <Sts1CobcSw/Periphery/Eps.hpp>
#include <Sts1CobcSw/Periphery/Fram.hpp>
#include <Sts1CobcSw/Periphery/TemperatureSensor.hpp>
#include <Sts1CobcSw/ProgramId/ProgramId.hpp>
#include <Sts1CobcSw/Telemetry/TelemetryMemory.hpp>
#include <Sts1CobcSw/Telemetry/TelemetryRecord.hpp>
#include <Sts1CobcSw/Utility/ErrorDetectionAndCorrection.hpp>
#include <Sts1CobcSw/Utility/RealTime.hpp>
#include <Sts1CobcSw/Utility/RodosTime.hpp>
#include <Sts1CobcSw/Utility/TimeTypes.hpp>

#include <strong_type/affine_point.hpp>
#include <strong_type/difference.hpp>
#include <strong_type/type.hpp>

#include <rodos_no_using_namespace.h>

#include <cstdint>


namespace sts1cobcsw
{
constexpr auto stackSize = 4'000U;
constexpr auto telemetryThreadPeriod = 30 * s;


[[nodiscard]] auto CollectTelemetryData() -> TelemetryRecord;


class TelemetryThread : public RODOS::StaticThread<stackSize>
{
public:
    TelemetryThread() : StaticThread("TelemetryThread", telemetryThreadPriority)
    {
    }


private:
    void init() override
    {
    }


    void run() override
    {
        TIME_LOOP(0, value_of(telemetryThreadPeriod))
        {
            persistentVariables.template Store<"realTime">(CurrentRealTime());
            auto telemetryRecord = CollectTelemetryData();
            telemetryMemory.PushBack(telemetryRecord);
            telemetryTopic.publish(telemetryRecord);
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
        .eduShouldBePowered = persistentVariables.Load<"eduShouldBePowered">(),
        .eduIsAlive = eduIsAlive,
        .newEduResultIsAvailable = persistentVariables.Load<"newEduResultIsAvailable">(),
        .antennasShouldBeDeployed = persistentVariables.Load<"antennasShouldBeDeployed">(),
        .framIsWorking = fram::framIsWorking.Load(),
        .epsIsWorking = persistentVariables.Load<"epsIsWorking">(),
        .flashIsWorking = persistentVariables.Load<"flashIsWorking">(),
        .rfIsWorking = persistentVariables.Load<"rfIsWorking">(),
        // Booleans: byte 2:  and communication
        .lastTelecommandIdWasInvalid = persistentVariables.Load<"lastTelecommandIdWasInvalid">(),
        .lastTelecommandArgumentsWereInvalid =
            persistentVariables.Load<"lastTelecommandArgumentsWereInvalid">(),
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
        .lastTelecommandId = persistentVariables.Load<"lastTelecommandId">()};
}
}
