#pragma once


#include <Sts1CobcSw/FramSections/Section.hpp>
#include <Sts1CobcSw/FramSections/SubsectionInfo.hpp>
#include <Sts1CobcSw/FramSections/Subsections.hpp>
#include <Sts1CobcSw/Periphery/Fram.hpp>
#include <Sts1CobcSw/Utility/StringLiteral.hpp>


namespace sts1cobcsw
{
inline constexpr auto framMemory = fram::Section<fram::Address(0), fram::memorySize>();
inline constexpr auto persistentVariablesSize = fram::Size(100);
inline constexpr auto framSections =
    fram::Subsections<framMemory,
                      fram::SubsectionInfo<"persistentVariables0", persistentVariablesSize>,
                      fram::SubsectionInfo<"persistentVariables1", persistentVariablesSize>,
                      fram::SubsectionInfo<"persistentVariables2", persistentVariablesSize>,
                      fram::SubsectionInfo<"eduProgramQueue", fram::Size(20 * 8)>,
                      fram::SubsectionInfo<"eduProgramStatusHistory", fram::Size(50 * 7)>,
                      fram::SubsectionInfo<"testMemory", fram::Size(1000)>,
                      fram::SubsectionInfo<"telemetry", fram::Size(26'168 * 40)>>();
}
