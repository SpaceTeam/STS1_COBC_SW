#include <Tests/UnitTests/UnitTestThread.hpp>

#include <Sts1CobcSw/FramSections/ProgramQueue.hpp>
#include <Sts1CobcSw/FramSections/Section.hpp>
#include <Sts1CobcSw/Periphery/Fram.hpp>
#include <Sts1CobcSw/Periphery/FramMock.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>

#include <bit>

#include "Sts1CobcSw/Serial/Serial.hpp"
#include "Sts1CobcSw/Utility/Debug.hpp"

namespace fram = sts1cobcsw::fram;
using sts1cobcsw::operator""_b;  // NOLINT(misc-unused-using-decls)
using sts1cobcsw::totalSerialSize;

inline constexpr auto dummySection =
    sts1cobcsw::Section<fram::Address(0), fram::Size(3 * 4 + 12)>{};

auto programQueueChar = sts1cobcsw::ProgramQueue<char, dummySection, 2>();

auto RunUnitTest() -> void
{
    using fram::ram::memory;
    fram::ram::SetAllDoFunctions();

    Require(programQueueChar.Size() == 0);
    Require(not programQueueChar.Full());
    Require(programQueueChar.Empty());

    fram::framIsWorking.Store(true);
    memory.fill(0x00_b);  // Clear memory

    // Test PushBack for char
    Require(programQueueChar.PushBack('a'));
    Require(programQueueChar.Size() == 1);
    Require(programQueueChar.Get(0U) == 'a');

    Require(programQueueChar.PushBack('b'));
}
