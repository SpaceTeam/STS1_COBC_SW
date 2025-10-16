#pragma once

#include <Sts1CobcSw/Vocabulary/Time.hpp>


namespace sts1cobcsw
{
class FileTransferTimer
{
public:
    FileTransferTimer() = default;
    explicit FileTransferTimer(Duration duration, RodosTime fileTransferWindowEnd);

    auto Set(Duration duration) -> void;
    auto UpdateFileTransferWindowEnd(RodosTime fileTransferWindowEnd) -> void;

    [[nodiscard]] auto ExpirationTime() const -> RodosTime;
    [[nodiscard]] auto HasExpired() const -> bool;


private:
    RodosTime expirationTime_ = RodosTime(0);
    RodosTime fileTransferWindowEnd_ = RodosTime(0);
};
}
