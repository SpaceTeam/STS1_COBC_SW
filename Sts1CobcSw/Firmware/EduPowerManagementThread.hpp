#pragma once


#include <Sts1CobcSw/Vocabulary/Time.hpp>

#include <strong_type/difference.hpp>
#include <strong_type/type.hpp>

#include <utility>


namespace sts1cobcsw
{
// FIXME: const value correct?
inline constexpr auto eduPowerManagementThreadStartDelay = 15 * s;


auto ResetEdu() -> void;
}
