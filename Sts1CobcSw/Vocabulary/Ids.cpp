#include <Sts1CobcSw/Vocabulary/Ids.hpp>


namespace sts1cobcsw
{
auto ToCZString(PartitionId partitionId) -> char const *
{
    switch(partitionId)
    {
        case PartitionId::primary:
            return "primary";
        case PartitionId::secondary1:
            return "secondary1";
        case PartitionId::secondary2:
            return "secondary2";
    }
    return "unknown";
}


auto ToClosestSecondaryPartitionId(SerialBuffer<PartitionId> id) -> Byte
{
    auto popCount = std::popcount(static_cast<std::uint8_t>(id[0]));
    if(popCount > 4)
    {
        return 0xFF_b;  // NOLINT(*magic-numbers)
    }
    return 0x00_b;
};
}
