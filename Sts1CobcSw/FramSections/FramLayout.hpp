#pragma once


#include <Sts1CobcSw/FramSections/Section.hpp>
#include <Sts1CobcSw/FramSections/SubsectionInfo.hpp>
#include <Sts1CobcSw/FramSections/Subsections.hpp>
#include <Sts1CobcSw/Periphery/Fram.hpp>
#include <Sts1CobcSw/Utility/StringLiteral.hpp>


namespace sts1cobcsw
{
inline constexpr auto framMemory = Section<fram::Address(0), fram::memorySize>();
inline constexpr auto persistentVariablesSize = fram::Size(100);
inline constexpr auto framSections =
    Subsections<framMemory,
                SubsectionInfo<"persistentVariables0", persistentVariablesSize>,
                SubsectionInfo<"persistentVariables1", persistentVariablesSize>,
                SubsectionInfo<"persistentVariables2", persistentVariablesSize>,
                SubsectionInfo<"eduProgramQueue", fram::Size(20 * 8)>,
                SubsectionInfo<"eduProgramStatusHistory", fram::Size(50 * 7)>,
                SubsectionInfo<"testMemory", fram::Size(1000)>,
                SubsectionInfo<"telemetry", fram::Size(26'168 * 40)>>();
}
