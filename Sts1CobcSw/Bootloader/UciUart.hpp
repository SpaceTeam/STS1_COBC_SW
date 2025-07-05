#pragma once


namespace sts1cobcsw::bootloader::uciuart
{
auto Initialize() -> void;
auto Reset() -> void;
auto Write(char character) -> void;
auto Write(char const * string) -> void;
}
