#include <Sts1CobcSw/FirmwareManagement/FirmwareManagement.hpp>
#include <Sts1CobcSw/Outcome/Outcome.hpp>
#include <Sts1CobcSw/RodosTime/RodosTime.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Utility/Crc32.hpp>
#include <Sts1CobcSw/Utility/Span.hpp>
#include <Sts1CobcSw/Vocabulary/Time.hpp>

#include <rodos_no_using_namespace.h>

#include <etl/vector.h>
#include <array>
#include <cinttypes>
#include <cstddef>
#include <cstdint>
#include <span>
#include <algorithm>
#include <limits>


namespace sts1cobcsw
{
using RODOS::PRINTF;

namespace
{
constexpr std::uint32_t imageBytes = 128U * 1024U;
constexpr std::uint32_t imageWithoutCrcBytes = imageBytes - 4U;

auto ProgramImage(fw::Partition const & partition) -> Result<void>
{
    auto image = etl::vector<Byte, imageBytes>{};

    auto const imageLengthSerialized = Serialize(imageWithoutCrcBytes);
    image.insert(image.end(), imageLengthSerialized.begin(), imageLengthSerialized.end());

    std::uint32_t const dataLength = imageWithoutCrcBytes - static_cast<std::uint32_t>(imageLengthSerialized.size());
    image.resize(image.size() + dataLength);
    std::uint8_t pattern = 0x00U;
    for(std::uint32_t i = 0; i < dataLength; ++i)
    {
        image[image.size() - dataLength + i] = static_cast<Byte>(pattern);
        pattern = static_cast<std::uint8_t>(pattern + 1U);
    }

    auto crc = ComputeCrc32(sts1cobcsw::Span(image));
    auto const crcSerialized = Serialize(crc);
    image.insert(image.end(), crcSerialized.begin(), crcSerialized.end());

    OUTCOME_TRY(fw::Erase(partition.flashSector));
    OUTCOME_TRY(fw::Program(partition.startAddress, Span(image)));
    return outcome_v2::success();
}
}


namespace
{
class CheckFirmwareIntegrityTimingTest : public RODOS::StaticThread<200*1024>
{
public:
    CheckFirmwareIntegrityTimingTest() : StaticThread("CheckFirmwareIntegrityTimingTest")
    {}


private:
    void init() override {}


    void run() override
    {
        PRINTF("\nCheckFirmwareIntegrity timing manual test\n");
        PRINTF("Preparing firmware image (length w/o CRC = %u bytes, total = %u bytes)\n",
               static_cast<unsigned int>(imageWithoutCrcBytes),
               static_cast<unsigned int>(imageBytes));

        auto const & partition = fw::secondaryPartition2;

        auto programResult = ProgramImage(partition);
        if(programResult.has_error())
        {
            PRINTF("Programming image failed: %s\n", ToCZString(programResult.error()));
            return;
        }

        static constexpr int iterations = 50;
        std::int64_t sumUs = 0;
        std::int64_t minUs = std::numeric_limits<std::int64_t>::max();
        std::int64_t maxUs = std::numeric_limits<std::int64_t>::min();

        for(int i = 0; i < iterations; ++i)
        {
            auto const t0 = CurrentRodosTime();
            auto result = fw::CheckFirmwareIntegrity(partition.startAddress);
            auto const t1 = CurrentRodosTime();
            if(result.has_error())
            {
                PRINTF("Integrity check failed: %s\n", ToCZString(result.error()));
                return;
            }
            auto const duration = (t1 - t0);
            auto const durationUs = static_cast<std::int64_t>(duration / us);
            sumUs += durationUs;
            minUs = std::min(minUs, durationUs);
            maxUs = std::max(maxUs, durationUs);
        }

        auto const avgUs = sumUs / iterations;

        PRINTF("\nCheckFirmwareIntegrity timing results over %d runs:\n",
               iterations);
        PRINTF("  min: %" PRId64 " us\n", minUs);
        PRINTF("  avg: %" PRId64 " us\n", avgUs);
        PRINTF("  max: %" PRId64 " us\n", maxUs);
    }
} checkFirmwareIntegrityTimingTest;
}
}


