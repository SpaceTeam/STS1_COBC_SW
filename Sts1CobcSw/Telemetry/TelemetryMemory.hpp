#pragma once


#include <Sts1CobcSw/FramSections/FramLayout.hpp>
#include <Sts1CobcSw/FramSections/FramRingArray.hpp>
#include <Sts1CobcSw/FramSections/Subsections.hpp>
#include <Sts1CobcSw/Telemetry/TelemetryRecord.hpp>


namespace sts1cobcsw
{
inline constexpr auto nCachedTelemetryRecords = 10;
inline constexpr auto telemetryMemory = FramRingArray<TelemetryRecord,
                                                      framSections.template Get<"telemetry">(),
                                                      nCachedTelemetryRecords>{};
}
