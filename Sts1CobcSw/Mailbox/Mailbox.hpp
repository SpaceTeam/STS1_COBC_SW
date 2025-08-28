#pragma once

#include <Sts1CobcSw/Outcome/Outcome.hpp>
#include <Sts1CobcSw/Vocabulary/Time.hpp>

#include <rodos_no_using_namespace.h>


namespace sts1cobcsw
{
template<typename Message>
class Mailbox
{
public:
    [[nodiscard]] auto IsEmpty() -> bool;
    [[nodiscard]] auto IsFull() -> bool;
    [[nodiscard]] auto SuspendUntilFullOr(RodosTime time) -> Result<void>;
    [[nodiscard]] auto SuspendUntilEmptyOr(RodosTime time) -> Result<void>;
    [[nodiscard]] auto Put(Message const & message) -> Result<void>;
    auto Overwrite(Message const & message) -> void;
    [[nodiscard]] auto Get() -> Result<Message>;
    [[nodiscard]] auto Peek() const -> Result<Message>;


private:
    Message message_ = {};
    bool isFull_ = false;

    RODOS::Thread * thread_ = nullptr;
    mutable RODOS::Semaphore semaphore_;
};
}

#include <Sts1CobcSw/Mailbox/Mailbox.ipp>  // IWYU pragma: keep
