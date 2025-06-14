#include <Sts1CobcSw/Edu/Types.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>
#include <Sts1CobcSw/Serial/UInt.hpp>
#include <Sts1CobcSw/Telemetry/TelemetryRecord.hpp>
#include <Sts1CobcSw/Vocabulary/ProgramId.hpp>
#include <Sts1CobcSw/Vocabulary/Time.hpp>

#include <catch2/catch_test_macros.hpp>

#include <bit>
#include <cstdint>
#include <utility>


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
        static constexpr auto originalRecord = TelemetryRecord{
            .eduShouldBePowered = 1,
            .eduIsAlive = 1,
            .newEduResultIsAvailable = 1,
            .antennasShouldBeDeployed = 1,
            .framIsWorking = 1,
            .epsIsWorking = 1,
            .flashIsWorking = 1,
            .rfIsWorking = 1,
            .lastMessageTypeIdWasInvalid = 1,
            .lastApplicationDataWasInvalid = 1,
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
            .cobcTemperature = 15,
            .rfTemperature = 16,
            // clang-format off
            .epsAdcData = {
                .adc4 = {
                    17U, 18U, 19U, 20U, 21U, 22U, 23U, 24U, 25U, 26U, 27U, 28U, 29U, 30U, 31U, 32U},
                .adc5 = {33U, 34U, 35U, 36U, 37U, 38U, 39U, 40U, 41U, 42U},
                .adc6 = {43U, 44U, 45U, 46U, 47U, 48U, 49U, 50U, 51U, 52U}},
            // clang-format on
            .rxBaudRate = 53,
            .txBaudRate = 54,
            .nCorrectableUplinkErrors = 55U,
            .nUncorrectableUplinkErrors = 56U,
            .nGoodTransferFrames = 57U,
            .nBadTransferFrames = 58U,
            .lastFrameSequenceNumber = 59U,
            .lastMessageTypeId = {60U, 61U},
        };
        auto serializedRecord = Serialize<std::endian::big>(originalRecord);
        auto deserializedRecord = Deserialize<std::endian::big, TelemetryRecord>(serializedRecord);
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
            auto originalRecord =
                TelemetryRecord{.eduShouldBePowered = (booleans1 & (1U << 0U)),
                                .eduIsAlive = (booleans1 & (1U << 1U)),
                                .newEduResultIsAvailable = (booleans1 & (1U << 2U)),
                                .antennasShouldBeDeployed = (booleans1 & 3U),
                                .framIsWorking = (booleans1 & (1U << 4U)),
                                .epsIsWorking = (booleans1 & (1U << 5U)),
                                .flashIsWorking = (booleans1 & (1U << 6U)),
                                .rfIsWorking = (booleans1 & (1U << 7U)),
                                .lastMessageTypeIdWasInvalid = (booleans2 & (1U << 0U)),
                                .lastApplicationDataWasInvalid = (booleans2 & (1U << 1U))};
            auto serializedRecord = Serialize<std::endian::big>(originalRecord);
            auto deserializedRecord =
                Deserialize<std::endian::big, TelemetryRecord>(serializedRecord);
            CHECK(originalRecord == deserializedRecord);
        }
    }
}
