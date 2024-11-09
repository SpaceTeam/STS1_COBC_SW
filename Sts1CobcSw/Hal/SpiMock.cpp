#include <Sts1CobcSw/Hal/SpiMock.hpp>
#include <Sts1CobcSw/Utility/RodosTime.hpp>

#include <rodos_no_using_namespace.h>


namespace sts1cobcsw::hal
{
namespace empty
{
auto DoInitialize([[maybe_unused]] std::uint32_t baudRate) -> void
{
}


auto DoRead([[maybe_unused]] void * data,
            [[maybe_unused]] std::size_t nBytes,
            [[maybe_unused]] Duration timeout) -> void
{
}


auto DoWrite([[maybe_unused]] void const * data,
             [[maybe_unused]] std::size_t nBytes,
             [[maybe_unused]] Duration timeout) -> void
{
}


auto DoTransferEnd() -> RodosTime
{
    return endOfTime;
}


auto DoBaudRate() -> std::int32_t
{
    return 0;
}
}
}
