#include <Sts1CobcSw/Hal/GpioPin.hpp>
#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/ThreadPriorities.hpp>
#include <Sts1CobcSw/TopicsAndSubscribers.hpp>

#include <rodos_no_using_namespace.h>

#include <cinttypes>


namespace sts1cobcsw
{
using RODOS::MILLISECONDS;

// TODO: Get a better estimation for the required stack size. We only have 128 kB of RAM.
constexpr auto stackSize = 2'000U;

auto ledGpioPin = hal::GpioPin(hal::led1Pin);
auto epsChargingGpioPin = hal::GpioPin(hal::epsChargingPin);
auto eduHeartbeatGpioPin = hal::GpioPin(hal::eduHeartbeatPin);

auto const edgeCounterThreshold = 4;


auto EduIsAlive() -> bool;


class EduHeartbeatThread : public RODOS::StaticThread<stackSize>
{
public:
    EduHeartbeatThread() : StaticThread("EduHeartbeatThread", eduHeartbeatThreadPriority)
    {
    }

private:
    void init() override
    {
        eduHeartbeatGpioPin.Direction(hal::PinDirection::in);
        ledGpioPin.Direction(hal::PinDirection::out);
        epsChargingGpioPin.Direction(hal::PinDirection::out);
        ledGpioPin.Reset();
        epsChargingGpioPin.Reset();
        // edu.Initialize();
    }


    void run() override
    {
        //        TIME_LOOP(0, 1*RODOS::SECONDS){
        //
        //            auto type = EduIsAlive();
        //            if(type) {
        //                RODOS::PRINTF("Edu is alive\n");
        //            } else {
        //                RODOS::PRINTF("Edu is dead\n");
        //            }
        //            eduIsAliveTopic.publish(type);
        //
        //        }

        // RODOS::AT(RODOS::END_OF_TIME);

        auto const heartbeatFrequency = 10;                     // Hz
        auto const samplingFrequency = 5 * heartbeatFrequency;  // Hz
        auto const samplingPeriod = 1'000 * MILLISECONDS / samplingFrequency;

        auto samplingCount = 0;
        auto heartbeatIsConstant = true;
        auto oldHeartbeat = eduHeartbeatGpioPin.Read();
        auto edgeCounter = 0;

        RODOS::PRINTF("Sampling period : %" PRIi64 "\n", samplingPeriod / MILLISECONDS);
        auto toggle = true;
        TIME_LOOP(0, samplingPeriod)
        {
            // Read current heartbeat value

            if(toggle)
            {
                epsChargingGpioPin.Set();
            }
            else
            {
                epsChargingGpioPin.Reset();
            }
            toggle = not toggle;

            auto heartbeat = eduHeartbeatGpioPin.Read();

            ++samplingCount;

            // If heartbeat was constant but is not anymore
            if(heartbeatIsConstant and (heartbeat != oldHeartbeat))
            {
                heartbeatIsConstant = false;
                // RODOS::PRINTF("Detected an edge \n");
                edgeCounter++;
            }

            if(edgeCounter == edgeCounterThreshold)
            {
                // RODOS::PRINTF("Edu is alive published to true\n");
                eduIsAliveTopic.publish(true);
                edgeCounter = 0;
            }
            // if(oldHeartbeat == heartbeat) {
            //    heartbeatIsConstant = true;
            //}

            oldHeartbeat = heartbeat;

            // Check if heartbeat is constant over a whole heartbeat period
            if(samplingCount == samplingFrequency / heartbeatFrequency)
            {
                if(heartbeatIsConstant)
                {
                    edgeCounter = 0;
                    // RODOS::PRINTF("Edu is alive published to false\n");
                    eduIsAliveTopic.publish(false);
                }
                heartbeatIsConstant = true;
                samplingCount = 0;
            }
        }
    }
} eduHeartbeatThread;


auto EduIsAlive() -> bool
{
    auto begin = RODOS::NOW();

    auto refHeartbeat = eduHeartbeatGpioPin.Read();
    auto edgeCounter = 0;
    auto const nEduHeartbeatReads = 1'000'000;
    for(auto i = 0; i < nEduHeartbeatReads; ++i)
    {
        auto heartbeat = eduHeartbeatGpioPin.Read();
        if(heartbeat != refHeartbeat)
        {
            edgeCounter++;
            refHeartbeat = heartbeat;
            if(edgeCounter >= edgeCounterThreshold)
            {
                return true;
            }
        }
    }

    auto executionTime = RODOS::NOW() - begin;
    RODOS::PRINTF("Execution Time of EduIsAlive (ns) : %" PRIi64 "\n", executionTime);
    RODOS::PRINTF("Execution Time of EduIsAlive (ms) : %" PRIi64 "\n",
                  executionTime / RODOS::MILLISECONDS);
    return false;
}
/*
constexpr auto dummyThreadPriority = 100;
class DummyThread : public RODOS::StaticThread<>
{
public:
    // TODO: Add this to all the other threads as well
    DummyThread() : StaticThread("DummyThread", dummyThreadPriority)
    {
    }

private:
    void init()
    {
        edu.Initialize();
    }

    void run()
    {
        edu.TurnOn();
        RODOS::AT(RODOS::NOW() + 25 * RODOS::SECONDS);

        RODOS::PRINTF("Hello From DummyThread Timeloop\n");
        auto programId = 0_u16;
        auto timestamp = 5_u16;
        auto timeout = 10_i16;

        auto executeProgramData = ExecuteProgramData{
            .programId = programId, .timestamp = timestamp, .timeout = timeout};
        edu.ExecuteProgram(executeProgramData);

        RODOS::AT(RODOS::NOW() + 11 * RODOS::SECONDS);

        [[maybe_unused]] auto status = edu.GetStatus();
        status = edu.GetStatus();

        edu.ReturnResult();
    }

} dummyThread;
*/
}
