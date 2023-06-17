#include <Sts1CobcSw/Hal/GpioPin.hpp>
#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/Periphery/Edu.hpp>
#include <Sts1CobcSw/ThreadPriorities.hpp>
#include <Sts1CobcSw/TopicsAndSubscribers.hpp>

#include <type_safe/types.hpp>

#include <rodos_no_using_namespace.h>


namespace sts1cobcsw
{
using RODOS::MILLISECONDS;

// TODO: Get a better estimation for the required stack size. We only have 128 kB of RAM.
constexpr auto stackSize = 2'000U;

auto ledGpioPin = hal::GpioPin(hal::ledPin);
auto epsChargingGpioPin = hal::GpioPin(hal::epsChargingPin);
auto eduHeartbeatGpioPin = hal::GpioPin(hal::eduHeartbeatPin);

// periphery::Edu edu = periphery::Edu();

auto constexpr edgeCounterThreshold = 4;


auto EduIsAlive() -> bool;


class EduHeartbeatThread : public RODOS::StaticThread<stackSize>
{
public:
    // TODO: Add this to all the other threads as well
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
        namespace ts = type_safe;
        using ts::operator""_i;
        using ts::operator""_isize;

        constexpr auto heartbeatFrequency = 10_isize;                     // Hz
        constexpr auto samplingFrequency = 5_isize * heartbeatFrequency;  // Hz
        constexpr auto samplingPeriod = 1'000_isize * MILLISECONDS / samplingFrequency;

        auto samplingCount = 0_i;
        ts::bool_t heartbeatIsConstant = true;
        auto oldHeartbeat = eduHeartbeatGpioPin.Read();
        auto edgeCounter = 0_i;

        RODOS::PRINTF("Sampling period : %lld\n", samplingPeriod.get() / RODOS::MILLISECONDS);
        auto toggle = true;
        TIME_LOOP(0, samplingPeriod.get())
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
                samplingCount = 0_i;
            }
        }
    }
} eduHeartbeatThread;


auto EduIsAlive() -> bool
{
    auto begin = RODOS::NOW();

    auto refHeartbeat = eduHeartbeatGpioPin.Read();
    auto heartbeat = eduHeartbeatGpioPin.Read();
    auto edgeCounter = 0;
    for(int i = 0; i < 1'000'000; ++i)
    {
        heartbeat = eduHeartbeatGpioPin.Read();
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
    RODOS::PRINTF("Execution Time of EduIsAlive (ns) : %lld\n", executionTime);
    RODOS::PRINTF("Execution Time of EduIsAlive (ms) : %lld\n",
                  executionTime / RODOS::MILLISECONDS);
    return false;
}
// TODO: Get back to the inline thread variable definition (like above) for all threads
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
        auto queueId = 5_u16;
        auto timeout = 10_i16;

        auto executeProgramData = periphery::ExecuteProgramData{
            .programId = programId, .queueId = queueId, .timeout = timeout};
        edu.ExecuteProgram(executeProgramData);

        RODOS::AT(RODOS::NOW() + 11 * RODOS::SECONDS);

        [[maybe_unused]] auto status = edu.GetStatus();
        status = edu.GetStatus();

        edu.ReturnResult();
    }

} dummyThread;
*/

}
