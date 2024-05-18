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
        // Check if semaphore is occupied by another thread
        if((owner != 0) && (owner != caller))
        {
            // Avoid priority inversion
            if(callerPriority > owner->getPriority())
            {
                owner->setPriority(callerPriority);
            }
            // Sleep until wake up by leave
            while(owner != 0 && owner != caller)
            {
                RODOS::Thread::suspendCallerUntil(RODOS::END_OF_TIME, this);
            }
            ownerEnterCnt = 0;
        }
        owner = caller;
        ownerPriority = callerPriority;
        ownerEnterCnt++;
    }                 // end of prio_ceiling
    caller->yield();  // wating with prio_ceiling, maybe some one more important wants to work?
}


void LinuxSemaphore::Leave()
{
    RODOS::Thread * caller = RODOS::Thread::getCurrentThread();
    RODOS::Thread * waiter = 0;

    if(owner != caller)
    {  // User Programm error: What to do? Nothing!
        return;
    }

    ownerEnterCnt--;
    if(ownerEnterCnt > 0)
    {  // same thread made multiple enter()
        return;
    }

    {
        RODOS::PRIORITY_CEILER_IN_SCOPE();
        owner = 0;
        waiter = RODOS::Thread::findNextWaitingFor(this);

        if(waiter != 0)
        {
            owner = waiter;  // set new owner, so that no other thread can grep the semaphore before
                             // thread switch
            waiter->resume();
        }
    }  // end of PRIORITY_CEILER,

    //   priority of current thread might have been increased in enter() due to a semaphore access
    //   of another thread with higher priority
    //   If so, restore the original priority
    RODOS::Thread::setPrioCurrentRunner(ownerPriority);
    ownerPriority = 0;

    /*
     * In case resume performs no yield:
     * To speed up semaphore handling the waiter-thread should be executed asap
     * -> that can be achieved by pause caller-thread with yield()
     * If there is a scheduler-call (triggered by timer) before the yield-call, it is not
     * necessary any more!
     * -> to avoid unnecessary yield the variable ownerPriority will be checked
     * -> if ownerPriority is still 0 there was no thread-switch before
     * -> Of course there can be a thread switch directly before the yield-call. In this case
     *    we have to accept the delay caused by the unnecessary yield-call.
     */
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
        // Check if semaphore is occupied by another thread
        if((owner != 0) && (owner != caller))
        {
            return false;
        }
        owner = caller;
        ownerPriority = callerPriority;
        ownerEnterCnt++;
    }                 // end of prio_ceiling
    caller->yield();  // wating with prio_ceiling, maybe some one more important wants to work?
    return true;
}
}
