#pragma once


namespace sts1cobcsw
{
// Short-interval watchdog timer
namespace wdt
{
auto Initialize() -> void;
auto Feed() -> void;
}


// Long-interval resetdog timer
namespace rdt
{
auto Initialize() -> void;
auto Feed() -> void;
}
}
