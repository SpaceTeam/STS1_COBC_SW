#include <Sts1CobcSw/EduCommunicationErrorThread.hpp>
#include <Sts1CobcSw/EduListenerThread.hpp>
#include <Sts1CobcSw/EduProgramQueue.hpp>
#include <Sts1CobcSw/EduProgramQueueThread.hpp>
#include <Sts1CobcSw/EduProgramStatusHistory.hpp>
#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/Hal/PinNames.hpp>
#include <Sts1CobcSw/Periphery/Edu.hpp>
#include <Sts1CobcSw/Periphery/EduNames.hpp>
#include <Sts1CobcSw/Periphery/EduStructs.hpp>
#include <Sts1CobcSw/Periphery/Rf.hpp>  // Only for MFT
#include <Sts1CobcSw/ThreadPriorities.hpp>

#include <type_safe/narrow_cast.hpp>
#include <type_safe/types.hpp>

#include <rodos.h>

#include "Sts1CobcSw/TopicsAndSubscribers.hpp"


namespace sts1cobcsw
{
constexpr auto timeLoopPeriod = 1 * RODOS::SECONDS;

// Can't use auto here since GCC throws an error about conflicting declarations otherwise :(
hal::GpioPin eduUpdateGpioPin(hal::eduUpdatePin);


class EduListenerThread : public StaticThread<>
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
        // Only for MFT
        RODOS::PRINTF("Initialize RF module\n");
        periphery::rf::Initialize(periphery::rf::TxType::packet);


        TIME_LOOP(0, timeLoopPeriod)
        {
            // RODOS::PRINTF("[EduListenerThread] Start of TimeLoop Iteration\n");
            auto eduHasUpdate = (eduUpdateGpioPin.Read() == hal::PinState::set);

            auto eduIsAlive = false;
            eduIsAliveBufferForListener.get(eduIsAlive);
            // RODOS::PRINTF("[EduListenerThread] Read eduHasUpdate pin\n");

            // TODO: Check if EDU is alive
            if(eduIsAlive and eduHasUpdate)
            {
                // RODOS::PRINTF("[EduListenerThread] Edu is alive and has an update\n");
                // Communicate with EDU

                auto status = edu.GetStatus();
                // RODOS::PRINTF("EduStatus : %d, EduErrorcode %d\n", status.statusType,
                // status.errorCode);

                if(status.errorCode != periphery::EduErrorCode::success
                   and status.errorCode != periphery::EduErrorCode::successEof)
                {
                    // RODOS::PRINTF("[EduListenerThread] GetStatus() error code : %d.\n",
                    // status.errorCode);
                    // RODOS::PRINTF(
                    //   "[EduListenerThread] Edu communication error after call to
                    //   GetStatus().\n");
                    ResumeEduErrorCommunicationThread();
                }
                else
                {
                    // RODOS::PRINTF("[EduListenerThread] Call to GetStatus() resulted in
                    // success.\n");
                }

                switch(status.statusType)
                {
                    case periphery::EduStatusType::programFinished:
                    {
                        // Program has finished
                        // Find the correspongind queueEntry and update it, then resume EDU queue
                        // thread
                        auto eduProgramStatusHistoryEntry =
                            FindEduProgramStatusHistoryEntry(status.programId, status.queueId);
                        if(status.exitCode == 0)
                        {
                            eduProgramStatusHistoryEntry.status =
                                EduProgramStatus::programExecutionSucceeded;
                        }
                        else
                        {
                            eduProgramStatusHistoryEntry.status =
                                EduProgramStatus::programExecutionFailed;
                        }
                        ResumeEduProgramQueueThread();
                        break;
                    }
                    case periphery::EduStatusType::resultsReady:
                    {
                        // EDU wants to send result file
                        // Send return result to EDU, Communicate, and interpret the results to
                        // update the S&H Entry from 3 or 4 to 5.
                        RODOS::PRINTF("Call to Return result with program id = %d, queueId = %d\n",
                                      status.programId,
                                      status.queueId);  // NOLINT
                        auto resultsInfo = edu.ReturnResult(
                            {.programId = status.programId, .queueId = status.queueId});
                        auto errorCode = resultsInfo.errorCode;
                        if(errorCode != periphery::EduErrorCode::success
                           and errorCode != periphery::EduErrorCode::successEof)
                        {
                            /*
                            RODOS::PRINTF(
                                "[EduListenerThread] Error Code From ReturnResult() : %d.\n",
                                errorCode);
                            RODOS::PRINTF(
                                "[EduListenerThread] Communication error after call to "
                                "ReturnResult().\n");
                                */
                            ResumeEduErrorCommunicationThread();
                        }
                        else
                        {
                            // RODOS::PRINTF(
                            //    "[EduListenerThread] Call to ReturnResults() resulted in "
                            //    "success.\n");
                        }
                        // break;

                        auto eduProgramStatusHistoryEntry =
                            FindEduProgramStatusHistoryEntry(status.programId, status.queueId);
                        // TODO: Pretty sure that there is a .put() or something like that missing
                        // here and the status is actually never updated in the ring buffer.
                        eduProgramStatusHistoryEntry.status =
                            EduProgramStatus::resultFileTransfered;
                        break;
                    }
                    case periphery::EduStatusType::invalid:
                    case periphery::EduStatusType::noEvent:
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
