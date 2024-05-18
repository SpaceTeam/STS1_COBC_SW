#pragma once

#include <rodos_no_using_namespace.h>

#include <cstdint>


// TODO: Add header to iwyu
namespace sts1cobcsw::utility
{

class LinuxSemaphore
{
private:
    RODOS::Thread * volatile owner;

    int volatile ownerEnterCnt;

protected:
    volatile int32_t ownerPriority;

    void * context;

public:
    LinuxSemaphore();

    auto Enter() -> void;

    auto Leave() -> void;

    auto TryEnter() -> bool;
};
}
