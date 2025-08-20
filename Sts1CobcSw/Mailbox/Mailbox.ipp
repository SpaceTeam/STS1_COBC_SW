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
    auto endTime = CurrentRodosTime() + duration;
    
    // Initial check - if already full, return immediately
    semaphore_.enter();
    if(isFull_)
    {
        semaphore_.leave();
        return outcome_v2::success();
    }
    
    // Setup for suspension
    thread_ = RODOS::Thread::getCurrentThread();
    RODOS::PRIORITY_CEILER_IN_SCOPE();
    semaphore_.leave();
    
    // Loop only around the suspend/resume cycle
    while(true)
    {
        auto currentTime = CurrentRodosTime();
        if(currentTime >= endTime)
        {
            thread_ = nullptr;
            return ErrorCode::timeout;
        }
        
        auto remainingTime = endTime - currentTime;
        auto result = SuspendUntilResumed(remainingTime);
        
        if(result.has_error())
        {
            thread_ = nullptr;
            return result;
        }
        
        // Check condition after resume
        semaphore_.enter();
        if(isFull_)
        {
            semaphore_.leave();
            thread_ = nullptr;
            return outcome_v2::success();
        }
        semaphore_.leave();
    }
}


template<typename Message>
auto Mailbox<Message>::SuspendUntilEmpty(Duration duration) -> Result<void>
{
    auto endTime = CurrentRodosTime() + duration;
    
    // Initial check - if already empty, return immediately
    semaphore_.enter();
    if(!isFull_)
    {
        semaphore_.leave();
        return outcome_v2::success();
    }
    
    // Setup for suspension
    thread_ = RODOS::Thread::getCurrentThread();
    RODOS::PRIORITY_CEILER_IN_SCOPE();
    semaphore_.leave();
    
    // Loop only around the suspend/resume cycle
    while(true)
    {
        auto currentTime = CurrentRodosTime();
        if(currentTime >= endTime)
        {
            thread_ = nullptr;
            return ErrorCode::timeout;
        }
        
        auto remainingTime = endTime - currentTime;
        auto result = SuspendUntilResumed(remainingTime);
        
        if(result.has_error())
        {
            thread_ = nullptr;
            return result;
        }
        
        // Check condition after resume
        semaphore_.enter();
        if(!isFull_)
        {
            semaphore_.leave();
            thread_ = nullptr;
            return outcome_v2::success();
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
    if(!isFull_)
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
    if(!isFull_)
    {
        return ErrorCode::empty;
    }
    return message_;
}
}
