#include <Sts1CobcSw/Edu/Edu.hpp>
#include <Sts1CobcSw/Edu/Enums.hpp>
#include <Sts1CobcSw/Edu/ProgramStatusHistory.hpp>
#include <Sts1CobcSw/Edu/Structs.hpp>
#include <Sts1CobcSw/EduCommunicationErrorThread.hpp>
#include <Sts1CobcSw/EduListenerThread.hpp>
#include <Sts1CobcSw/EduProgramQueueThread.hpp>
#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/ThreadPriorities.hpp>
#include <Sts1CobcSw/TopicsAndSubscribers.hpp>

#include <rodos_no_using_namespace.h>


namespace sts1cobcsw
{
constexpr auto timeLoopPeriod = 1 * RODOS::SECONDS;

// TODO: This should also go to Edu.hpp/.cpp
hal::GpioPin eduUpdateGpioPin(hal::eduUpdatePin);


class EduListenerThread : public RODOS::StaticThread<>
{
public:
    EduListenerThread() : StaticThread("EduListenerThread", eduListenerThreadPriority)
    {
    }


private:
    void init() override
    {
        eduUpdateGpioPin.Direction(hal::PinDirection::in);
    }


    void run() override
    {
        TIME_LOOP(0, timeLoopPeriod)
        {
            // RODOS::PRINTF("[EduListenerThread] Start of TimeLoop Iteration\n");
            auto eduHasUpdate = (eduUpdateGpioPin.Read() == hal::PinState::set);

            auto eduIsAlive = false;
            eduIsAliveBufferForListener.get(eduIsAlive);
            // RODOS::PRINTF("[EduListenerThread] Read eduHasUpdate pin\n");

            // TODO: Check if edu is alive
            if(eduIsAlive and eduHasUpdate)
            {
                // RODOS::PRINTF("[EduListenerThread] Edu is alive and has an update\n");
                // Communicate with EDU

                auto status = eduUnit.GetStatus();
                // RODOS::PRINTF("EduStatus : %d, EduErrorcode %d\n", status.statusType,
                // status.errorCode);

                if(status.errorCode != edu::ErrorCode::success
                   and status.errorCode != edu::ErrorCode::successEof)
                {
                    // RODOS::PRINTF("[EduListenerThread] GetStatus() error code : %d.\n",
                    // status.errorCode);
                    // RODOS::PRINTF(
                    //   "[EduListenerThread] Edu communication error after call to
                    //   GetStatus().\n");
                    ResumeEduCommunicationErrorThread();
                }
                else
                {
                    // RODOS::PRINTF("[EduListenerThread] Call to GetStatus() resulted in
                    // success.\n");
                }

                switch(status.statusType)
                {
                    case edu::StatusType::programFinished:
                    {
                        // Program has finished
                        // Find the correspongind queueEntry and update it, then resume edu queue
                        // thread
                        if(status.exitCode == 0)
                        {
                            edu::UpdateProgramStatusHistory(
                                status.programId,
                                status.queueId,
                                edu::ProgramStatus::programExecutionSucceeded);
                        }
                        else
                        {
                            edu::UpdateProgramStatusHistory(
                                status.programId,
                                status.queueId,
                                edu::ProgramStatus::programExecutionFailed);
                        }
                        ResumeEduProgramQueueThread();
                        break;
                    }
                    case edu::StatusType::resultsReady:
                    {
                        // Edu wants to send result file
                        // Send return result to Edu, Communicate, and interpret the results to
                        // update the S&H Entry from 3 or 4 to 5.
                        auto resultsInfo = eduUnit.ReturnResult();
                        auto errorCode = resultsInfo.errorCode;
                        if(errorCode != edu::ErrorCode::success
                           and errorCode != edu::ErrorCode::successEof)
                        {
                            /*
                            RODOS::PRINTF(
                                "[EduListenerThread] Error Code From ReturnResult() : %d.\n",
                                errorCode);
                            RODOS::PRINTF(
                                "[EduListenerThread] Communication error after call to "
                                "ReturnResult().\n");
                                */
                            ResumeEduCommunicationErrorThread();
                        }
                        else
                        {
                            // RODOS::PRINTF(
                            //    "[EduListenerThread] Call to ReturnResults() resulted in "
                            //    "success.\n");
                        }
                        // break;

                        edu::UpdateProgramStatusHistory(status.programId,
                                                        status.queueId,
                                                        edu::ProgramStatus::resultFileTransfered);
                        break;
                    }
                    case edu::StatusType::invalid:
                    case edu::StatusType::noEvent:
                    {
                        break;
                    }
                }
            }
            // RODOS::PRINTF("[EduListenerThread] Edu Has no uppdate\n");
        }
    }
} eduListenerThread;
}
