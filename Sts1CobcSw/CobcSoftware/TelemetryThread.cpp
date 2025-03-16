#include <Sts1CobcSw/CobcSoftware/RfCommunicationThread.hpp>
#include <Sts1CobcSw/CobcSoftware/TelemetryCollection.hpp>
#include <Sts1CobcSw/CobcSoftware/ThreadPriorities.hpp>
#include <Sts1CobcSw/CobcSoftware/TopicsAndSubscribers.hpp>
#include <Sts1CobcSw/FramSections/FramLayout.hpp>
#include <Sts1CobcSw/FramSections/PersistentVariables.hpp>
#include <Sts1CobcSw/Telemetry/TelemetryMemory.hpp>
#include <Sts1CobcSw/Telemetry/TelemetryRecord.hpp>
#include <Sts1CobcSw/Utility/RealTime.hpp>

#include <rodos_no_using_namespace.h>


namespace sts1cobcsw
{
constexpr auto stackSize = 4'000U;
constexpr auto telemetryThreadPeriod = 30 * s;


class TelemetryThread : public RODOS::StaticThread<stackSize>
{
public:
    TelemetryThread() : StaticThread("TelemetryThread", telemetryThreadPriority)
    {
    }


private:
    void init() override
    {
    }


    void run() override
    {
        TIME_LOOP(0, value_of(telemetryThreadPeriod))
        {
            persistentVariables.template Store<"realTime">(CurrentRealTime());
            auto telemetryRecord = CollectTelemetryData();
            telemetryMemory.PushBack(telemetryRecord);
            telemetryTopic.publish(telemetryRecord);
            ResumeRfCommunicationThread();
        }
    }
} telemetryThread;
}
