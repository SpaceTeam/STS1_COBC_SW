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
            TelemetryRecord{.fwChecksumsAreOk = true,
                            .eduShouldBePowered = true,
                            .eduHeartBeats = true,
                            .newResultIsAvailable = true,
                            .antennasShouldBeDeployed = true,
                            .framIsWorking = true,
                            .epsIsWorking = true,
                            .flashIsWorking = true,
                            .rfIsWorking = true,
                            .lastTeleCommandIdWasInvalid = true,
                            .lastTeleCommandArgumentsWereInvalid = true,
                            .nResetsSinceRf = 1U,
                            .activeSecondaryFwPartition = 2,
                            .backupSecondaryFwPartition = 3,
                            .eduProgramQueueIndex = 4,
                            .programIdOfCurrentEduProgramQueueEntry = ProgramId(5),
                            .nEduCommunicationErrors = 7U,
                            .nTotalResets = 8U,
                            .lastResetReason = 9U,
                            .rodosTimeInSeconds = 10,
                            .realTime = RealTime(11),
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
                            .lastReceivedCommandId = 30U};
        auto serializedRecord = Serialize(originalRecord);
        auto deserializedRecord = Deserialize<TelemetryRecord>(serializedRecord);
        CHECK(originalRecord == deserializedRecord);
    }

    SECTION("All booleans are serialized and deserialized correctly")
    {
        static constexpr auto nBooleans1 = 4U;
        static constexpr auto nBooleans2 = 7U;
        static constexpr auto nBooleans = nBooleans1 + nBooleans2;
        // We loop over all records where a single boolean set to true
        for(auto i = 0U; i < nBooleans; ++i)
        {
            auto booleans1 = static_cast<std::uint8_t>((1U << i) & ((1U << nBooleans1) - 1U));
            auto booleans2 = static_cast<std::uint8_t>((1U << i) >> nBooleans1);
            auto originalRecord = TelemetryRecord{
                .fwChecksumsAreOk = static_cast<bool>(booleans1 & 1U),
                .eduShouldBePowered = static_cast<bool>(booleans1 & (1U << 1U)),
                .eduHeartBeats = static_cast<bool>(booleans1 & (1U << 2U)),
                .newResultIsAvailable = static_cast<bool>(booleans1 & (1U << 3U)),
                .antennasShouldBeDeployed = static_cast<bool>(booleans2 & 1U),
                .framIsWorking = static_cast<bool>(booleans2 & (1U << 1U)),
                .epsIsWorking = static_cast<bool>(booleans2 & (1U << 2U)),
                .flashIsWorking = static_cast<bool>(booleans2 & (1U << 3U)),
                .rfIsWorking = static_cast<bool>(booleans2 & (1U << 4U)),
                .lastTeleCommandIdWasInvalid = static_cast<bool>(booleans2 & (1U << 5U)),
                .lastTeleCommandArgumentsWereInvalid = static_cast<bool>(booleans2 & (1U << 6U))};
            auto serializedRecord = Serialize(originalRecord);
            auto deserializedRecord = Deserialize<TelemetryRecord>(serializedRecord);
            CHECK(originalRecord == deserializedRecord);
        }
    }
}
