#include <Sts1CobcSw/Edu/Types.hpp>
#include <Sts1CobcSw/ProgramId/ProgramId.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>
#include <Sts1CobcSw/Telemetry/TelemetryRecord.hpp>
#include <Sts1CobcSw/Utility/TimeTypes.hpp>

#include <catch2/catch_test_macros.hpp>
#include <strong_type/type.hpp>


using sts1cobcsw::Deserialize;
using sts1cobcsw::ProgramId;
using sts1cobcsw::RealTime;
using sts1cobcsw::Serialize;
using sts1cobcsw::TelemetryRecord;
using sts1cobcsw::edu::ProgramStatus;


TEST_CASE("(De-)Serialization of TelemetryRecord")
{
    static constexpr auto originalRecord = TelemetryRecord{
        .nResetsSinceRf = 1U,
        .activeSecondaryFwPartition = 2,
        .backupSecondaryFwPartition = 3,
        .fwChecksumsAreOk = true,
        .eduShouldBePowered = true,
        .eduHeartBeats = true,
        .eduProgramQueueIndex = 4,
        .programIdOfCurrentProgramQueueEntry = ProgramId(5),
        .programIdOfLatestStatusAndHistoryEntry = ProgramId(6),
        .statusOfLatestStatusAndHistoryEntry = ProgramStatus::programRunning,
        .newResultIsAvailable = true,
        .nEduCommunicationErrors = 7U,
        .nTotalResets = 8U,
        .lastResetReason = 9U,
        .rodosTimeInSeconds = 10,
        .realTime = RealTime(11),
        .antennasShouldBeDeployed = true,
        .framIsWorking = true,
        .epsIsWorking = true,
        .flashIsWorking = true,
        .rfIsWorking = true,
        .nFlashErrors = 12U,
        .nRfErrors = 13U,
        .nFileSystemErrors = 14U,
        .batteryPackVoltage = 15U,
        .batteryCenterTapVoltage = 16U,
        .batteryTemperature = 17U,
        .cobcTemperature = 18U,
        .cubeSatBusVoltage = 19U,
        .sidepanelXPlusTemperature = 20U,
        .sidepanelYPlusTemperature = 21U,
        .sidepanelYMinusTemperature = 22U,
        .sidepanelZPlusTemperature = 23U,
        .sidepanelZMinusTemperature = 24U,
        .rfBaudRate = 25U,
        .nCorrectableUplinkErrors = 26U,
        .nUncorrectableUplinkErrors = 27U,
        .nBadRfpackets = 28U,
        .nGoodRfpackets = 29U,
        .lastReceivedCommandId = 30U,
        .lastReceivedCommandIdWasInvalid = true,
        .lastReceivedCommandArgumentsWereInvalid = true,
    };
    auto serializedRecord = Serialize(originalRecord);
    auto deserializedRecord = Deserialize<TelemetryRecord>(serializedRecord);
    CHECK(originalRecord == deserializedRecord);
}
