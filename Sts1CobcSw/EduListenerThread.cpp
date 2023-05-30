#include <Sts1CobcSw/EduCommunicationErrorThread.hpp>
#include <Sts1CobcSw/EduListenerThread.hpp>
#include <Sts1CobcSw/EduProgramQueue.hpp>
#include <Sts1CobcSw/EduProgramQueueThread.hpp>
#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/Hal/PinNames.hpp>
#include <Sts1CobcSw/Periphery/Edu.hpp>
#include <Sts1CobcSw/Periphery/EduNames.hpp>
#include <Sts1CobcSw/Periphery/EduStructs.hpp>
#include <Sts1CobcSw/ThreadPriorities.hpp>

#include <type_safe/narrow_cast.hpp>
#include <type_safe/types.hpp>

#include <rodos.h>

#include "Sts1CobcSw/TopicsAndSubscribers.hpp"


namespace sts1cobcsw
{
hal::GpioPin eduUpdateGpioPin(hal::eduUpdatePin);


constexpr auto timeLoopPeriod = 1 * RODOS::SECONDS;

auto FindStatusAndHistoryEntry(std::uint16_t programId, std::uint16_t queueId) -> StatusHistoryEntry
{
    auto counter = 0;
    auto statusHistoryEntry = StatusHistoryEntry{};
    do
    {
        statusHistory.get(statusHistoryEntry);
        // RODOS::PRINTF("%d,%d vs %d,%d\n", statusHistoryEntry.programId,
        // statusHistoryEntry.queueId, programId, queueId);
    } while(statusHistoryEntry.queueId != queueId or statusHistoryEntry.programId != programId);

    return statusHistoryEntry;
}


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
                        // Find the correspongind queueEntry and update it, then resume edu queue
                        // thread

                        auto statusHistoryEntry =
                            FindStatusAndHistoryEntry(status.programId, status.queueId);

                        if(status.exitCode == 0)
                        {
                            statusHistoryEntry.status = ProgramStatus::programExecutionSucceeded;
                        }
                        else
                        {
                            statusHistoryEntry.status = ProgramStatus::programExecutionFailed;
                        }
                        ResumeEduProgramQueueThread();

                        break;
                    }

                    case periphery::EduStatusType::resultsReady:
                    {
                        // Edu wants to send result file
                        // Send return result to Edu, Communicate, and interpret the results to
                        // update the S&H Entry from 3 or 4 to 5.
                        auto resultsInfo = edu.ReturnResult();
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

                        auto statusHistoryEntry =
                            FindStatusAndHistoryEntry(status.programId, status.queueId);
                        statusHistoryEntry.status = ProgramStatus::resultFileTransfered;


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
