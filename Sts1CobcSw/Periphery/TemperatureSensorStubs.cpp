#include <Sts1CobcSw/Periphery/TemperatureSensor.hpp>  // IWYU pragma: associated


namespace sts1cobcsw::rftemperaturesensor
{
auto Initialize() -> void
{
}


auto Read() -> std::uint16_t
{
    return 0x1234U;  // NOLINT(*magic-numbers)
}
}
