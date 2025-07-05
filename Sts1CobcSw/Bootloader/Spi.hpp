#pragma once

namespace sts1cobcsw::spi
{
auto Initialize() -> void;
auto Reset() -> void;
auto Write(unsigned char character) -> void;
auto Read(char * character) -> void;
}
