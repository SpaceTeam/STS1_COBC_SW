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
    cancelled,
    abandoned,
};


inline constexpr auto unknownTransactionSequenceNumber = std::numeric_limits<std::uint16_t>::max();


struct FileTransferInfo
{
    FileTransferStatus status = FileTransferStatus::inactive;
    std::uint16_t sequenceNumber = unknownTransactionSequenceNumber;

    auto operator==(FileTransferInfo const &) const -> bool = default;
};
}
