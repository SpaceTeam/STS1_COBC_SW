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
    auto const imageLengthSerialized = Serialize(imageWithoutCrcBytes);
    OUTCOME_TRY(fw::Program(partition.startAddress, Span(imageLengthSerialized)));

    auto crc = ComputeCrc32(Span(imageLengthSerialized));
    auto buffer = etl::vector<Byte, 128>{};
    for(std::size_t i = sizeof(imageWithoutCrcBytes); i < imageWithoutCrcBytes;)
    {
        std::size_t const nBytes = std::min<std::size_t>(buffer.capacity(), imageWithoutCrcBytes - i);
        buffer.resize(nBytes);
        fw::Read(partition.startAddress + i, Span(&buffer));
        crc = ComputeCrc32(crc, Span(buffer));
        i += nBytes;
    }

    OUTCOME_TRY(fw::Program(partition.startAddress + imageWithoutCrcBytes, Span(Serialize(crc))));

    return outcome_v2::success();
}
}


namespace
{
class CheckFirmwareIntegrityTimingTest : public RODOS::StaticThread<>
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
