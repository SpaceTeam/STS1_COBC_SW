#include <Sts1CobcSw/Utility/Time.hpp>

#include <cinttypes>


namespace sts1cobcsw::utility
{
//! @brief Print UTC system time in human readable format
void PrintFormattedSystemUtc()
{
    std::int32_t year = 0;
    std::int32_t month = 0;
    std::int32_t day = 0;
    std::int32_t hour = 0;
    std::int32_t min = 0;
    double sec = 0;

    auto sysUtc = RODOS::sysTime.getUTC();
    RODOS::TimeModel::localTime2Calendar(sysUtc, year, month, day, hour, min, sec);
    RODOS::PRINTF("DateUTC(DD/MM/YYYY HH:MIN:SS) : %02" PRIi32 "/%02" PRIi32 "/%02" PRIi32
                  " %02" PRIi32 ":%02" PRIi32 ":%02.0f\n",
                  day,
                  month,
                  year,
                  hour,
                  min,
                  sec);
}
}
