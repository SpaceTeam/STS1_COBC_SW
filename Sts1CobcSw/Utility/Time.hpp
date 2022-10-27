#include <rodos_no_using_namespace.h>

#include <cstdint>


namespace sts1cobcsw::utility
{
//! Number of nanoseconds between 1st January 1970 and 1st January 2000
constexpr auto rodosUnixOffsetNanoseconds = 946'684'800 * RODOS::SECONDS;
//! Number of seconds between 1st January 1970 and 1st January 2000
constexpr auto rodosUnixOffsetDelay = 946'684'800;

//! @brief Print utc system time in human readable format
void PrintTime();

//! @brief Given a time in seconds since January 1st 1970, return a time in nanoseconds since
//! January 1st 2000.
[[nodiscard]] inline auto UnixToRodosTime(int32_t const unixTime)
{
    auto rodosTime = static_cast<int64_t>(unixTime);
    rodosTime = rodosTime * RODOS::SECONDS;
    rodosTime = rodosTime - rodosUnixOffsetNanoseconds;
    return rodosTime;
}

[[nodiscard]] inline auto GetUnixUtc() {

    auto systemUTC = RODOS::sysTime.getUTC();
    systemUTC += rodosUnixOffsetNanoseconds;
    auto unixUtc = systemUTC / RODOS::SECONDS;
    return static_cast<int32_t>(unixUtc);
}

}
