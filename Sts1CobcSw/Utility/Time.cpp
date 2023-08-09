#include <Sts1CobcSw/Utility/Time.hpp>

#include <cinttypes>


namespace sts1cobcsw::utility
{
//! @brief Print UTC system time in human readable format
void PrintFormattedSystemUtc()
{
    int32_t year = 0;
    int32_t month = 0;
    int32_t day = 0;
    int32_t hour = 0;
    int32_t min = 0;
    double sec = 0;

    auto sysUTC = RODOS::sysTime.getUTC();
    RODOS::TimeModel::localTime2Calendar(sysUTC, year, month, day, hour, min, sec);
    // Print only seconds by casting it to long
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
