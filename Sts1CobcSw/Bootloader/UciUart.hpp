#pragma once


namespace sts1cobcsw::uciuart
{
auto Initialize() -> void;
auto Reset() -> void;
auto Write(char character) -> void;
auto Write(char const * string) -> void;

[[gnu::format(printf, 1, 2)]] auto PrintF(char const * format, ...) -> void;
}
