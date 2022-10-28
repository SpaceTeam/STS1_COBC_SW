#include <Sts1CobcSw/Hal/GpioPin.hpp>
#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/TopicsAndSubscribers.hpp>

#include <type_safe/types.hpp>

#include <rodos_no_using_namespace.h>


namespace sts1cobcsw
{
using RODOS::MILLISECONDS;

auto ledGpio = hal::GpioPin(hal::ledPin);
auto eduHeartBeatGpio = hal::GpioPin(hal::eduHeartbeatPin);


class EduHeartbeatThread : public RODOS::StaticThread<>
{
public:
    EduHeartbeatThread() : StaticThread("EduHeartbeat")
    {
    }

private:
    void init() override
    {
        eduHeartBeatGpio.Direction(hal::PinDirection::in);
        ledGpio.Direction(hal::PinDirection::out);
    }


    void run() override
    {
        namespace ts = type_safe;
        using ts::operator""_i;
        using ts::operator""_isize;

        auto samplingCount = 0_i;
        ts::bool_t heartbeatIsConstant = true;
        ts::bool_t oldHeartbeat = eduHeartBeatGpio.Read() == hal::PinState::set;
        constexpr auto heartbeatFrequency = 10_isize;                                     // Hz
        constexpr auto samplingFrequency = 5_isize * heartbeatFrequency;                  // Hz
        constexpr auto samplingPeriode = 1'000_isize / samplingFrequency * MILLISECONDS;  // ms

        TIME_LOOP(0, samplingPeriode.get())
        {
            ts::bool_t heartbeat = eduHeartBeatGpio.Read() == hal::PinState::set;
            ++samplingCount;

            if(heartbeatIsConstant and (heartbeat != oldHeartbeat))
            {
                heartbeatIsConstant = false;
                ledGpio.Set();
                eduIsAliveTopic.publish(true);
            }

            oldHeartbeat = heartbeat;

            // Check if heartbeat is constant over a whole heartbeat period
            if(samplingCount == samplingFrequency / heartbeatFrequency)
            {
                if(heartbeatIsConstant)
                {
                    ledGpio.Reset();
                    eduIsAliveTopic.publish(false);
                }
                heartbeatIsConstant = true;
                samplingCount = 0;
            }
        }
    }
};


auto const eduHeartbeatThread = EduHeartbeatThread();
}
