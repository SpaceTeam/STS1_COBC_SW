#include <Sts1CobcSw/Utility/Time.hpp>


namespace sts1cobcsw::utility
{
//! @brief Print utc system time in human readable format
void PrintTime()
{
    int32_t year = 0;
    int32_t month = 0;
    int32_t day = 0;
    int32_t hour = 0;
    int32_t min = 0;
    double sec = 0;

    auto sysUTC = RODOS::sysTime.getUTC();
    RODOS::TimeModel::localTime2Calendar(sysUTC, year, month, day, hour, min, sec);
    RODOS::PRINTF("DateUTC(DD/MM/YYYY HH:MIN:SS) : %ld/%ld/%ld %ld:%ld:%f\n",
                  day,    // NOLINT
                  month,  // NOLINT
                  year,   // NOLINT
                  hour,   // NOLINT
                  min,    // NOLINT
                  sec);
}
}
