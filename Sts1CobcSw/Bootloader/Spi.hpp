#pragma once

#include <Sts1CobcSw/Serial/Byte.hpp>

#include <span>


namespace sts1cobcsw::spi
{
auto Initialize() -> void;
auto Reset() -> void;
auto Write(Byte byte) -> void;
auto Write(std::span<Byte const> data) -> void;
auto WaitUntilTxComplete() -> void;
auto Read(std::span<Byte> data) -> void;
}
