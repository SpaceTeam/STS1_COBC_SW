#include <Sts1CobcSw/Bootloader/BusyWait.hpp>
#include <Sts1CobcSw/Bootloader/Leds.hpp>
#include <Sts1CobcSw/Bootloader/UciUart.hpp>


namespace uciuart = sts1cobcsw::uciuart;
namespace leds = sts1cobcsw::leds;


auto main() -> int
{
    leds::Initialize();
    leds::TurnOn();
    uciuart::Initialize();
    uciuart::Write("Hello from the bootloader!\n");
    uciuart::Reset();
    static constexpr auto delay = 500'000;
    while(true)
    {
        leds::TurnOff();
        sts1cobcsw::BusyWaitUs(delay);
        leds::TurnOn();
        sts1cobcsw::BusyWaitUs(delay);
    }
}
