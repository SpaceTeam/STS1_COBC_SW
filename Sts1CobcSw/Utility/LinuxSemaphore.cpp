#include <Sts1CobcSw/Utility/LinuxSemaphore.hpp>

#include <rodos_no_using_namespace.h>

namespace sts1cobcsw::utility
{


LinuxSemaphore::LinuxSemaphore() : owner(0), ownerEnterCnt(0), ownerPriority(0), context(0)
{
}


void LinuxSemaphore::Enter()
{
    RODOS::Thread * caller = RODOS::Thread::getCurrentThread();
    int32_t callerPriority = caller->getPriority();
    {
        RODOS::PRIORITY_CEILER_IN_SCOPE();
        if((owner != 0) && (owner != caller))
        {
            if(callerPriority > owner->getPriority())
            {
                owner->setPriority(callerPriority);
            }
            while(owner != 0 && owner != caller)
            {
                RODOS::Thread::suspendCallerUntil(RODOS::END_OF_TIME, this);
            }
            ownerEnterCnt = 0;
        }
        owner = caller;
        ownerPriority = callerPriority;
        ownerEnterCnt++;
    }
    caller->yield();
}


void LinuxSemaphore::Leave()
{
    RODOS::Thread * caller = RODOS::Thread::getCurrentThread();
    RODOS::Thread * waiter = 0;

    if(owner != caller)
    {
        return;
    }

    ownerEnterCnt--;
    if(ownerEnterCnt > 0)
    {
        return;
    }

    {
        RODOS::PRIORITY_CEILER_IN_SCOPE();
        owner = 0;
        waiter = RODOS::Thread::findNextWaitingFor(this);

        if(waiter != 0)
        {
            owner = waiter;
            waiter->resume();
        }
    }

    RODOS::Thread::setPrioCurrentRunner(ownerPriority);
    ownerPriority = 0;

    if((waiter != 0) && (ownerPriority == 0))
    {
        caller->yield();
    }
}


auto LinuxSemaphore::TryEnter() -> bool
{
    RODOS::Thread * caller = RODOS::Thread::getCurrentThread();
    int32_t callerPriority = caller->getPriority();
    {
        RODOS::PRIORITY_CEILER_IN_SCOPE();
        if((owner != 0) && (owner != caller))
        {
            return false;
        }
        owner = caller;
        ownerPriority = callerPriority;
        ownerEnterCnt++;
    }
    caller->yield();
    return true;
}
}
