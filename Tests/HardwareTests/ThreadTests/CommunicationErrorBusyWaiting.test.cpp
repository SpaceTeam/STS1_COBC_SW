//! @file
//! @brief A program to test busy waiting after an EDU communication error
//!
//! Previously, the heartbeat thread had a lower priority than the communication error thread, this
//! resulted in a bug where polling the heartbeat slowed down.

#include <Sts1CobcSw/EduCommunicationErrorThread.hpp>

#include <rodos_no_using_namespace.h>


namespace sts1cobcsw
{

class CommunicationErrorBusyWaitingTest : public RODOS::StaticThread<>
{
    void init() override
    {
    }


    void run() override
    {
        RODOS::PRINTF("Hello Busy Waiting Test!");
        ResumeEduErrorCommunicationThread();
        RODOS::AT(RODOS::END_OF_TIME);
    }
};


auto const communicationErrorBusyWaitingTest = CommunicationErrorBusyWaitingTest();
}
