#include <Sts1CobcSw/Edu/Types.hpp>
#include <Sts1CobcSw/ProgramId/ProgramId.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>
#include <Sts1CobcSw/Telemetry/TelemetryRecord.hpp>
#include <Sts1CobcSw/Utility/TimeTypes.hpp>

#include <catch2/catch_test_macros.hpp>
#include <strong_type/type.hpp>

#include <cstdint>
#include <string>


using sts1cobcsw::Deserialize;
using sts1cobcsw::ProgramId;
using sts1cobcsw::RealTime;
using sts1cobcsw::Serialize;
using sts1cobcsw::TelemetryRecord;
using sts1cobcsw::edu::ProgramStatus;


TEST_CASE("(De-)Serialization of TelemetryRecord")
{
    SECTION("Example record is serialized and deserialized correctly")
    {
        static constexpr auto originalRecord =
            TelemetryRecord{.eduShouldBePowered = true,
                            .eduIsAlive = true,
                            .newEduResultIsAvailable = true,
                            .antennasShouldBeDeployed = true,
                            .framIsWorking = true,
                            .epsIsWorking = true,
                            .flashIsWorking = true,
                            .rfIsWorking = true,
                            .lastTelecommandIdWasInvalid = true,
                            .lastTelecommandArgumentsWereInvalid = true,
                            .nTotalResets = 1U,
                            .nResetsSinceRf = 2U,
                            .activeSecondaryFwPartition = 3,
                            .backupSecondaryFwPartition = 4,
                            .eduProgramQueueIndex = 5U,
                            .programIdOfCurrentEduProgramQueueEntry = ProgramId(6),
                            .nEduCommunicationErrors = 7U,
                            .lastResetReason = 8U,
                            .rodosTimeInSeconds = 9,
                            .realTime = RealTime(10),
                            .nFirmwareChecksumErrors = 11U,
                            .nFlashErrors = 12U,
                            .nRfErrors = 13U,
                            .nFileSystemErrors = 14U,
                            .batteryPackVoltage = 15U,
                            .batteryCenterTapVoltage = 16U,
                            .batteryTemperature = 17U,
                            .cobcTemperature = 18U,
                            .rfTemperature = 19U,
                            .cubeSatBusVoltage = 20U,
                            .sidepanelXPlusTemperature = 21U,
                            .sidepanelYPlusTemperature = 22U,
                            .sidepanelYMinusTemperature = 23U,
                            .sidepanelZPlusTemperature = 24U,
                            .sidepanelZMinusTemperature = 25U,
                            .rxBaudRate = 26,
                            .txBaudRate = 27,
                            .nCorrectableUplinkErrors = 28U,
                            .nUncorrectableUplinkErrors = 29U,
                            .nGoodTransferFrames = 30U,
                            .nBadTransferFrames = 31U,
                            .lastTelecommandId = 32U};
        auto serializedRecord = Serialize(originalRecord);
        auto deserializedRecord = Deserialize<TelemetryRecord>(serializedRecord);
        CHECK(originalRecord == deserializedRecord);
    }

    SECTION("All booleans are serialized and deserialized correctly")
    {
        static constexpr auto nBooleans1 = 8U;
        static constexpr auto nBooleans2 = 2U;
        static constexpr auto nBooleans = nBooleans1 + nBooleans2;
        // We loop over all records where a single boolean set to true
        for(auto i = 0U; i < nBooleans; ++i)
        {
            auto booleans1 = static_cast<std::uint8_t>((1U << i) & ((1U << nBooleans1) - 1U));
            auto booleans2 = static_cast<std::uint8_t>((1U << i) >> nBooleans1);
            auto originalRecord = TelemetryRecord{
                .eduShouldBePowered = static_cast<bool>(booleans1 & (1U << 0U)),
                .eduIsAlive = static_cast<bool>(booleans1 & (1U << 1U)),
                .newEduResultIsAvailable = static_cast<bool>(booleans1 & (1U << 2U)),
                .antennasShouldBeDeployed = static_cast<bool>(booleans1 & 3U),
                .framIsWorking = static_cast<bool>(booleans1 & (1U << 4U)),
                .epsIsWorking = static_cast<bool>(booleans1 & (1U << 5U)),
                .flashIsWorking = static_cast<bool>(booleans1 & (1U << 6U)),
                .rfIsWorking = static_cast<bool>(booleans1 & (1U << 7U)),
                .lastTelecommandIdWasInvalid = static_cast<bool>(booleans2 & (1U << 0U)),
                .lastTelecommandArgumentsWereInvalid = static_cast<bool>(booleans2 & (1U << 1U))};
            auto serializedRecord = Serialize(originalRecord);
            auto deserializedRecord = Deserialize<TelemetryRecord>(serializedRecord);
            CHECK(originalRecord == deserializedRecord);
        }
    }
}
