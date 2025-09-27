#pragma once


#include <cstdint>
#include <numeric>


namespace sts1cobcsw
{
enum class FileTransferStatus : std::uint8_t
{
    inactive,
    sending,
    receiving,
    completed,
    canceled,
    abandoned,
};


inline constexpr auto unknownTransactionSequenceNumber = std::numeric_limits<std::uint16_t>::max();
}
