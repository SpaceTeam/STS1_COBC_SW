#pragma once

#include <Sts1CobcSw/SingleBuffer/SingleBuffer.hpp>

#include <Sts1CobcSw/RodosTime/RodosTime.hpp>


namespace sts1cobcsw
{
template<typename T>
auto SingleBuffer<T>::IsEmpty() -> bool
{
    return !isFull_;
}


template<typename T>
auto SingleBuffer<T>::IsFull() -> bool
{
    return isFull_;
}


template<typename T>
auto SingleBuffer<T>::SuspendUntilFull(Duration duration) -> Result<void>
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
    auto result = SuspendUntilResumed(duration);
    thread_ = nullptr;
    return result;
}


template<typename T>
auto SingleBuffer<T>::SuspendUntilEmpty(Duration duration) -> Result<void>
{
    semaphore_.enter();
    if(!isFull_)
    {
        semaphore_.leave();
        return outcome_v2::success();
    }
    thread_ = RODOS::Thread::getCurrentThread();
    RODOS::PRIORITY_CEILER_IN_SCOPE();
    semaphore_.leave();
    auto result = SuspendUntilResumed(duration);
    thread_ = nullptr;
    return result;
}


template<typename T>
auto SingleBuffer<T>::Put(T const & data) -> Result<void>
{
    auto protector = RODOS::ScopeProtector(&semaphore_);  // NOLINT(*readability-casting)
    if(isFull_)
    {
        return ErrorCode::full;
    }
    buffer_ = data;
    isFull_ = true;
    if(thread_ != nullptr)
    {
        thread_->resume();
    }
    return outcome_v2::success();
}


template<typename T>
auto SingleBuffer<T>::Get() -> Result<T>
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
    return buffer_;
}
}
