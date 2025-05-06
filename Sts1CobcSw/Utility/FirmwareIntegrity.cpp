#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Utility/Crc32.hpp>
#include <Sts1CobcSw/Utility/FirmwareIntegrity.hpp>
#include <Sts1CobcSw/Utility/Span.hpp>

#include <etl/vector.h>

#include <algorithm>
#include <cstddef>
#include <span>


namespace sts1cobcsw
{
constexpr auto maxFirmwareLength = 0x2'0000U;


// Partition structure:
// - Length: without CRC at the end, 4 bytes, in little endian
// - Data
// - Checksum: CRC-32 over length + data, in little endian
auto CheckFirmwareIntegrity(std::uintptr_t startAddress) -> Result<void>
{
    if((startAddress % sizeof(std::uint32_t)) != 0)
    {
        return ErrorCode::misaligned;
    }
    // NOLINTNEXTLINE(*reinterpret-cast, performance-no-int-to-ptr)
    auto length = *reinterpret_cast<std::uint32_t volatile *>(startAddress);
    // The length includes the 4 bytes for itself so it must be >= 4. Since the CRC-32 value at the
    // end of the FW image must be aligned to 4 bytes, the length must also be a multiple of 4.
    auto lengthIsValid = sizeof(std::uint32_t) <= length and length <= maxFirmwareLength
                     and (length % sizeof(std::uint32_t)) == 0;
    if(not lengthIsValid)
    {
        return ErrorCode::invalidLength;
    }
    // NOLINTNEXTLINE(*reinterpret-cast, performance-no-int-to-ptr)
    auto bytes = std::span(reinterpret_cast<Byte const volatile *>(startAddress), length);
    auto buffer = etl::vector<Byte, 128>{};  // NOLINT(*magic-numbers)
    buffer.resize(sizeof(length));
    std::ranges::copy(bytes.first<sizeof(length)>(), buffer.begin());
    auto crc = ComputeCrc32(Span(buffer));
    auto lengthWithChecksum = length + sizeof(crc);
    for(auto i = sizeof(length); i < lengthWithChecksum;)
    {
        auto nBytes = std::min<std::size_t>(lengthWithChecksum - i, buffer.capacity());
        buffer.resize(nBytes);
        std::ranges::copy(bytes.subspan(i, nBytes), buffer.begin());
        crc = ComputeCrc32(crc, Span(buffer));
        i += nBytes;
    }
    if(crc != 0)
    {
        return ErrorCode::corrupt;
    }
    return outcome_v2::success();
}
}
