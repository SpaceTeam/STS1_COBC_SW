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
    auto IsEmpty() -> bool;
    auto IsFull() -> bool;
    auto SuspendUntilFull(Duration duration) -> Result<void>;
    auto SuspendUntilEmpty(Duration duration) -> Result<void>;
    auto Put(Message const & message) -> Result<void>;
    auto Get() -> Result<Message>;


private:
    Message message_ = {};
    bool isFull_ = false;

    RODOS::Thread * thread_ = nullptr;
    RODOS::Semaphore semaphore_;
};
}

#include <Sts1CobcSw/Mailbox/Mailbox.ipp>
