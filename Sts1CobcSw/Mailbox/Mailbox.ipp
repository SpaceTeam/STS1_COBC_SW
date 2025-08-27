#pragma once

#include <Sts1CobcSw/Mailbox/Mailbox.hpp>

#include <Sts1CobcSw/RodosTime/RodosTime.hpp>


namespace sts1cobcsw
{
template<typename Message>
auto Mailbox<Message>::IsEmpty() -> bool
{
    return !isFull_;
}


template<typename Message>
auto Mailbox<Message>::IsFull() -> bool
{
    return isFull_;
}


template<typename Message>
auto Mailbox<Message>::SuspendUntilFull(Duration duration) -> Result<void>
{
    semaphore_.enter();
    if(isFull_)
    {
        semaphore_.leave();
        return outcome_v2::success();
    }
    threadWaitingForFull_ = RODOS::Thread::getCurrentThread();
    RODOS::PRIORITY_CEILER_IN_SCOPE();
    semaphore_.leave();

    auto const timeoutAt = CurrentRodosTime() + duration;
    while(true)
    {
        auto const remainingDuration = timeoutAt - CurrentRodosTime();
        auto const result = SuspendUntilResumed(remainingDuration);
        semaphore_.enter();
        if(isFull_)
        {
            threadWaitingForFull_ = nullptr;
            semaphore_.leave();
            return outcome_v2::success();
        }
        if(result.has_error())
        {
            threadWaitingForFull_ = nullptr;
            semaphore_.leave();
            return result;
        }
        semaphore_.leave();
    }
}


template<typename Message>
auto Mailbox<Message>::SuspendUntilEmpty(Duration duration) -> Result<void>
{
    semaphore_.enter();
    if(!isFull_)
    {
        semaphore_.leave();
        return outcome_v2::success();
    }
    threadWaitingForEmpty_ = RODOS::Thread::getCurrentThread();
    RODOS::PRIORITY_CEILER_IN_SCOPE();
    semaphore_.leave();

    auto const timeoutAt = CurrentRodosTime() + duration;
    while(true)
    {
        auto const remainingDuration = timeoutAt - CurrentRodosTime();
        auto const result = SuspendUntilResumed(remainingDuration);
        semaphore_.enter();
        if(!isFull_)
        {
            threadWaitingForEmpty_ = nullptr;
            semaphore_.leave();
            return outcome_v2::success();
        }
        if(result.has_error())
        {
            threadWaitingForEmpty_ = nullptr;
            semaphore_.leave();
            return result;
        }
        semaphore_.leave();
    }
}


template<typename Message>
auto Mailbox<Message>::Put(Message const & message) -> Result<void>
{
    auto protector = RODOS::ScopeProtector(&semaphore_);  // NOLINT(*readability-casting)
    if(isFull_)
    {
        return ErrorCode::full;
    }
    message_ = message;
    isFull_ = true;
    if(threadWaitingForFull_ != nullptr)
    {
        threadWaitingForFull_->resume();
    }
    return outcome_v2::success();
}


template<typename Message>
auto Mailbox<Message>::Overwrite(Message const & message) -> void
{
    auto protector = RODOS::ScopeProtector(&semaphore_);
    message_ = message;
    isFull_ = true;
    if(threadWaitingForFull_ != nullptr)
    {
        threadWaitingForFull_->resume();
    }
}


template<typename Message>
auto Mailbox<Message>::Get() -> Result<Message>
{
    auto protector = RODOS::ScopeProtector(&semaphore_);  // NOLINT(*readability-casting)
    if(!isFull_)
    {
        return ErrorCode::empty;
    }
    isFull_ = false;
    if(threadWaitingForEmpty_ != nullptr)
    {
        threadWaitingForEmpty_->resume();
    }
    return message_;
}


template<typename Message>
auto Mailbox<Message>::Peek() const -> Result<Message>
{
    auto protector = RODOS::ScopeProtector(&semaphore_);
    if(!isFull_)
    {
        return ErrorCode::empty;
    }
    return message_;
}
}
