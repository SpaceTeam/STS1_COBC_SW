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
// #include <Sts1CobcSw/Periphery/Rf.hpp>  // Only for MFT
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
        // // Only for MFT/HAF
        // RODOS::PRINTF("Initializing RF module\n");
        // periphery::rf::Initialize(periphery::rf::TxType::packet);


        TIME_LOOP(0, timeLoopPeriod)
        {
            auto eduHasUpdate = (eduUpdateGpioPin.Read() == hal::PinState::set);
            auto eduIsAlive = false;
            eduIsAliveBufferForListener.get(eduIsAlive);

            // TODO: Check if EDU is alive
            if(eduIsAlive and eduHasUpdate)
            {
                auto status = edu.GetStatus();
                if(status.errorCode != periphery::EduErrorCode::success
                   and status.errorCode != periphery::EduErrorCode::successEof)
                {
                    ResumeEduErrorCommunicationThread();
                }

                switch(status.statusType)
                {
                    case periphery::EduStatusType::programFinished:
                    {
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
                        RODOS::PRINTF("Calling ReturnResult(program id = %d, queueId = %d)\n",
                                      status.programId,
                                      status.queueId);  // NOLINT
                        auto resultsInfo = edu.ReturnResult(
                            {.programId = status.programId, .queueId = status.queueId});
                        if(resultsInfo.errorCode != periphery::EduErrorCode::success
                           and resultsInfo.errorCode != periphery::EduErrorCode::successEof)
                        {
                            ResumeEduErrorCommunicationThread();
                        }

                        auto eduProgramStatusHistoryEntry =
                            FindEduProgramStatusHistoryEntry(status.programId, status.queueId);
                        // TODO: Pretty sure that there is a .put() or something like that missing
                        // here and the status is actually never updated in the ring buffer.
                        eduProgramStatusHistoryEntry.status =
                            EduProgramStatus::resultFileTransfered;

                        // // Only for MFT
                        // RODOS::PRINTF("\n");
                        // // RODOS::PRINTF("Sending call sign ...\n");
                        // // periphery::rf::TransmitData(reinterpret_cast<std::uint8_t const *>(
                        // //                                 periphery::rf::portableCallSign.data()),
                        // //                             periphery::rf::portableCallSign.size());
                        // // RODOS::PRINTF("Sending 'test' ...\n");
                        // // std::uint8_t testMessage[] = "test";
                        // // periphery::rf::TransmitData(&testMessage[0], std::size(testMessage));

                        // RODOS::PRINTF("Sending result file ...\n");
                        // // Setting the TX type again is necessary because the beacon is Morsed
                        // periphery::rf::SetTxType(periphery::rf::TxType::packet);
                        // periphery::rf::TransmitData(
                        //     reinterpret_cast<std::uint8_t const *>(periphery::cepDataBuffer.data()),
                        //     resultsInfo.resultSize.get());
                        // RODOS::PRINTF("  Done\n");
                        // RODOS::PRINTF("\n");

                        break;
                    }
                    case periphery::EduStatusType::invalid:
                    case periphery::EduStatusType::noEvent:
                    {
                        break;
                    }
                }
            }
        }
    }
} eduListenerThread;
}
