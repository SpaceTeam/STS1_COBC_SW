#pragma once

#include <Sts1CobcSw/Outcome/Outcome.hpp>
#include <Sts1CobcSw/Vocabulary/Time.hpp>

#include <rodos_no_using_namespace.h>


namespace sts1cobcsw
{
template<typename T>
class SingleBuffer
{
public:
    auto IsEmpty() -> bool;
    auto IsFull() -> bool;
    auto SuspendUntilFull(Duration duration) -> Result<void>;
    auto SuspendUntilEmpty(Duration duration) -> Result<void>;
    auto Put(T const & data) -> Result<void>;
    auto Get() -> Result<T>;

private:
    T buffer_ = {};
    bool isFull_ = false;

    RODOS::Thread * thread_ = nullptr;
    static inline auto semaphore = RODOS::Semaphore{};
};
}

#include <Sts1CobcSw/SingleBuffer/SingleBuffer.ipp>
