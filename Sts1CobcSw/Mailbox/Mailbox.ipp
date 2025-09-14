#pragma once

#include <Sts1CobcSw/Mailbox/Mailbox.hpp>

#include <Sts1CobcSw/RodosTime/RodosTime.hpp>


namespace sts1cobcsw
{
template<typename Message>
auto Mailbox<Message>::IsEmpty() -> bool
{
    return not isFull_;
}


template<typename Message>
auto Mailbox<Message>::IsFull() -> bool
{
    return isFull_;
}


template<typename Message>
auto Mailbox<Message>::SuspendUntilFullOr(RodosTime time) -> Result<void>
{
    semaphore_.enter();
    if(isFull_)
    {
        semaphore_.leave();
        return outcome_v2::success();
    }
    thread_ = RODOS::Thread::getCurrentThread();
    RODOS::PRIORITY_CEILER_IN_SCOPE();
    semaphore_.leave();
    auto result = SuspendUntilResumedOr(time);
    thread_ = nullptr;
    return result;
}


template<typename Message>
auto Mailbox<Message>::SuspendUntilEmptyOr(RodosTime time) -> Result<void>
{
    semaphore_.enter();
    if(not isFull_)
    {
        semaphore_.leave();
        return outcome_v2::success();
    }
    thread_ = RODOS::Thread::getCurrentThread();
    RODOS::PRIORITY_CEILER_IN_SCOPE();
    semaphore_.leave();
    auto result = SuspendUntilResumedOr(time);
    thread_ = nullptr;
    return result;
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
    if(thread_ != nullptr)
    {
        thread_->resume();
    }
    return outcome_v2::success();
}


template<typename Message>
auto Mailbox<Message>::Overwrite(Message const & message) -> void
{
    auto protector = RODOS::ScopeProtector(&semaphore_);
    message_ = message;
    isFull_ = true;
    if(thread_ != nullptr)
    {
        thread_->resume();
    }
}


template<typename Message>
auto Mailbox<Message>::Get() -> Result<Message>
{
    auto protector = RODOS::ScopeProtector(&semaphore_);  // NOLINT(*readability-casting)
    if(not isFull_)
    {
        return ErrorCode::empty;
    }
    isFull_ = false;
    if(thread_ != nullptr)
    {
        thread_->resume();
    }
    return message_;
}


template<typename Message>
auto Mailbox<Message>::Peek() const -> Result<Message>
{
    auto protector = RODOS::ScopeProtector(&semaphore_);
    if(not isFull_)
    {
        return ErrorCode::empty;
    }
    return message_;
}
}
