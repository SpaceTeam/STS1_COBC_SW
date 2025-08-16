#include <Sts1CobcSw/Bootloader/Print.hpp>

#include <Sts1CobcSw/Bootloader/UciUart.hpp>

#include <cstdarg>
#include <cstdio>


namespace sts1cobcsw
{
auto PrintF(char const * format, ...) -> void  // NOLINT(cert-dcl50-cpp)
{
    static constexpr auto maxStringLength = 200;
    static constexpr auto bufferSize = maxStringLength + 1;  // +1 for null terminator
    static char buffer[bufferSize] = {};

    auto args = va_list{};
    va_start(args, format);
    auto stringLength = std::vsnprintf(&buffer[0], bufferSize, format, args);
    va_end(args);

    if(stringLength < 0)
    {
        uciuart::Write("Error: formatting failed\n");
        return;
    }
    if(stringLength >= bufferSize)
    {
        uciuart::Write("Error: formatted string too long\n");
        return;
    }
    uciuart::Write(&buffer[0]);
}
}
