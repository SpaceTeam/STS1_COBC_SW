#include <rodos_no_using_namespace.h>

namespace sts1cobcsw::util
{
//! Number of nanoseconds between 1st January 1970 and 1st January 2000
constexpr auto rodosUnixOffset = 946'684'800 * RODOS::SECONDS;
//! Number of seconds between 1st January 1970 and 1st January 2000
constexpr auto rodosUnixOffsetSeconds = 946'684'800;

//! @brief Print utc system time in human readable format
void PrintTime();

//! @brief Given a time in seconds since January 1st 1970, return a time in nanoseconds since
//! January 1st 2000.
[[nodiscard]] inline auto UnixToRodosTime(int32_t const unixTime)
{
    auto rodosTime = static_cast<int64_t>(unixTime);
    rodosTime = rodosTime * RODOS::SECONDS;
    rodosTime = rodosTime - util::rodosUnixOffset;
    return rodosTime;
}


}
