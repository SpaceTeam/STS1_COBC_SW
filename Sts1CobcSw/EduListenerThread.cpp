#include <Sts1CobcSw/EduCommunicationErrorThread.hpp>
#include <Sts1CobcSw/EduListenerThread.hpp>
#include <Sts1CobcSw/EduProgramQueue.hpp>
#include <Sts1CobcSw/EduProgramQueueThread.hpp>
#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/Hal/PinNames.hpp>
#include <Sts1CobcSw/Periphery/Edu.hpp>
#include <Sts1CobcSw/Periphery/EduNames.hpp>
#include <Sts1CobcSw/Periphery/EduStructs.hpp>

#include <type_safe/narrow_cast.hpp>
#include <type_safe/types.hpp>

#include <rodos.h>


namespace sts1cobcsw
{
hal::GpioPin eduUpdateGpioPin(hal::eduUpdatePin);
namespace ts = type_safe;

constexpr auto timeLoopPeriod = 1 * RODOS::SECONDS;
constexpr auto threadPriority = 100;

auto FindStatusAndHistoryEntry(std::uint16_t programId, std::uint16_t queueId) -> StatusHistoryEntry
{
    auto statusHistoryEntry = StatusHistoryEntry{};
    do
    {
        statusHistory.get(statusHistoryEntry);
    } while(statusHistoryEntry.queueId != queueId or statusHistoryEntry.programId != programId);

    return statusHistoryEntry;
}

class EduListenerThread : public StaticThread<>
{
public:
    EduListenerThread() : StaticThread("EduListenerThread", threadPriority)
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
            auto eduHasUpdate = (eduUpdateGpioPin.Read() == hal::PinState::set);

            if(eduHasUpdate)
            {
                // Communicate with EDU

                auto status = edu.GetStatus();

                if(status.errorCode != periphery::EduErrorCode::success)
                {
                    ResumeEduErrorCommunicationThread();
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
                        ResumeEduQueueThread();

                        break;
                    }

                    case periphery::EduStatusType::resultsReady:
                    {
                        // Edu wants to send result file
                        // Send return result to Edu, Communicate, and interpret the results to
                        // update the S&H Entry from 3 or 4 to 5.
                        auto resultsInfo = edu.ReturnResult();

                        auto errorCode = resultsInfo.errorCode;

                        if(errorCode != periphery::EduErrorCode::success)
                        {
                            ResumeEduErrorCommunicationThread();
                        }

                        auto statusHistoryEntry =
                            FindStatusAndHistoryEntry(status.programId, status.programId);
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
        }
    }
};

auto const eduListenerThread = EduListenerThread();
}
