#include <Tests/UnitTests/UnitTestThread.hpp>

#include <Sts1CobcSw/FramSections/Section.hpp>
#include <Sts1CobcSw/Periphery/Fram.hpp>
#include <Sts1CobcSw/Periphery/FramMock.hpp>
#include <Sts1CobcSw/FramSections/ProgramQueue.hpp>
#include "Sts1CobcSw/Serial/Serial.hpp"

namespace fram = sts1cobcsw::fram;

inline constexpr auto dummySection =
    sts1cobcsw::Section<fram::Address(0), fram::Size(30)>{};

auto programQueue = sts1cobcsw::ProgramQueue<char, dummySection, 2>();

auto RunUnitTest() -> void
{
    using fram::ram::memory;
    fram::ram::SetAllDoFunctions();

    Require(programQueue.Size() == 0);
    Require(programQueue.Full() == false);
    Require(programQueue.Empty() == true);

    Require(programQueue.PushBack('a'));

    // Failed
    Require(programQueue.Size() == 1);

    //Require(sts1cobcsw::serialSize<char> == 1);
}

