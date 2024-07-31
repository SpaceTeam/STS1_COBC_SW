#include <Sts1CobcSw/Edu/ProgramStatusHistory.hpp>
#include <Sts1CobcSw/Periphery/Fram.hpp>
#include <Sts1CobcSw/Periphery/FramMock.hpp>
#include <Sts1CobcSw/Periphery/FramRingBuffer.hpp>
#include <Sts1CobcSw/ProgramId/ProgramId.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>

#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

#include <etl/circular_buffer.h>

#include <algorithm>
#include <array>
#include <cstddef>

namespace fram = sts1cobcsw::fram;

using sts1cobcsw::operator""_b;  // NOLINT(misc-unused-using-decls)

TEST_CASE("Initial State ringbuffer")
{
    fram::RingBuffer<int, 10, fram::Address{0}> buffer;

    REQUIRE(buffer.Size() == 0);
    REQUIRE(buffer.Capacity() == 10);  // Fixed from 0 to 10 to reflect actual buffer capacity
}

TEST_CASE("FramRingBuffer Push function")
{
    fram::RingBuffer<int, 10, fram::Address{0}> buffer;

    buffer.Push(1);
    buffer.Push(2);
    buffer.Push(3);

    REQUIRE(buffer.Size() == 3);
}

TEMPLATE_TEST_CASE_SIG("FramRingBuffer Front Address",
                       "",
                       ((typename T, size_t S, fram::Address A), T, S, A),
                       (int, 10U, fram::Address{0}),
                       (int, 10U, fram::Address{31415}))
{
    fram::ram::SetAllDoFunctions();
    fram::ram::memory.fill(0x00_b);
    fram::Initialize();

    fram::RingBuffer<T, S, A> buffer;

    // FIXME: [] not working
    buffer.Push(10);
    REQUIRE(buffer[0] == 10);

    buffer.Push(20);
    REQUIRE(buffer[0] == 10);
    REQUIRE(buffer[1] == 20);

    buffer.Push(30);
    REQUIRE(buffer[0] == 10);
    REQUIRE(buffer[1] == 20);
    REQUIRE(buffer[2] == 30);
}

TEST_CASE("FramRingBuffer Back() and Front() methods")
{
    fram::ram::SetAllDoFunctions();
    fram::ram::memory.fill(0x00_b);
    fram::Initialize();

    auto buffer = fram::RingBuffer<int, 5, fram::Address{0}>();
    etl::circular_buffer<int, 5U> etlBuffer;

    // NOLINTNEXTLINE (readability-container-size-empty)
    CHECK(etlBuffer.size() == 0);
    CHECK(etlBuffer.max_size() == 5);
    CHECK(etlBuffer.capacity() == 5);

    buffer.Push(1);
    buffer.Push(2);
    REQUIRE(buffer.Front() == 1);
    buffer.Push(3);

    etlBuffer.push(1);
    etlBuffer.push(2);
    etlBuffer.push(3);

    REQUIRE(etlBuffer.size() == 3);
    REQUIRE(etlBuffer.capacity() == 5);
    REQUIRE(etlBuffer.back() == 3);
    REQUIRE(etlBuffer.front() == 1);

    REQUIRE(buffer.Size() == 3);
    REQUIRE(buffer.Capacity() == 5);
    REQUIRE(buffer.Back() == 3);
    REQUIRE(buffer.Front() == 1);

    etlBuffer.push(4);
    etlBuffer.push(5);
    // Overwrite
    etlBuffer.push(6);

    buffer.Push(4);
    buffer.Push(5);
    buffer.Push(6);

    REQUIRE(etlBuffer.front() == 2);
    REQUIRE(buffer.Front() == 2);

    REQUIRE(etlBuffer.back() == 6);
    REQUIRE(buffer.Back() == 6);

    REQUIRE(etlBuffer.size() == 5);
    REQUIRE(etlBuffer.capacity() == 5);
    REQUIRE(etlBuffer[0] == 2);
    REQUIRE(etlBuffer[1] == 3);
    REQUIRE(etlBuffer[2] == 4);
    REQUIRE(buffer[0] == 2);
    REQUIRE(buffer[1] == 3);
    REQUIRE(buffer[2] == 4);
}

TEST_CASE("FramRingBuffer Full and Empty conditions")
{
    fram::ram::SetAllDoFunctions();
    fram::ram::memory.fill(0x00_b);
    fram::Initialize();

    auto buffer = fram::RingBuffer<int, 3, fram::Address(0)>{};

    REQUIRE(buffer.Size() == 0);
    REQUIRE(buffer.Capacity() == 3);

    buffer.Push(1);
    buffer.Push(2);
    buffer.Push(3);

    REQUIRE(buffer.Size() == 3);
    REQUIRE(buffer.Front() == 1);
    REQUIRE(buffer.Back() == 3);

    buffer.Push(4);  // Overwrite the oldest element

    REQUIRE(buffer.Size() == 3);
    REQUIRE(buffer.Front() == 2);
    REQUIRE(buffer.Back() == 4);

    buffer.Push(10);

    REQUIRE(buffer.Size() == 3);
    REQUIRE(buffer.Front() == 3);
    REQUIRE(buffer.Back() == 10);
}

TEST_CASE("FramRingBuffer and ETL Circular Buffer")
{
    fram::ram::SetAllDoFunctions();
    fram::ram::memory.fill(0x00_b);
    fram::Initialize();

    fram::RingBuffer<int, 5U, fram::Address{0}> framBuffer{};
    etl::circular_buffer<int, 5U> etlBuffer;

    for(int i = 0; i < 5; ++i)
    {
        framBuffer.Push(i);
        etlBuffer.push(i);
    }

    REQUIRE(framBuffer.Size() == etlBuffer.size());
    for(size_t i = 0; i < framBuffer.Size(); ++i)
    {
        REQUIRE(framBuffer[i] == etlBuffer[i]);
    }

    framBuffer.Push(5);
    etlBuffer.push(5);

    REQUIRE(framBuffer.Front() == etlBuffer.front());
    REQUIRE(framBuffer.Back() == etlBuffer.back());
}

TEST_CASE("FramRingBuffer Stress Test")
{
    fram::ram::SetAllDoFunctions();
    fram::ram::memory.fill(0x00_b);
    fram::Initialize();

    auto buffer = fram::RingBuffer<int, 10000, fram::Address{0}>();

    for(int i = 0; i < 10000; ++i)
    {
        buffer.Push(i);
    }

    REQUIRE(buffer.Size() == 10000);

    for(size_t i = 0; i < 10000; ++i)
    {
        REQUIRE(buffer[i] == static_cast<int>(i));
    }
}

TEST_CASE("Custom Type")
{
    fram::ram::SetAllDoFunctions();
    fram::ram::memory.fill(0x00_b);
    fram::Initialize();


    fram::RingBuffer<sts1cobcsw::edu::ProgramStatusHistoryEntry, 10U, fram::Address{0}> buffer;

    auto pshEntry = sts1cobcsw::edu::ProgramStatusHistoryEntry{
        .programId = sts1cobcsw::ProgramId(0),
        .startTime = 0,
        .status = sts1cobcsw::edu::ProgramStatus::programRunning};

    // some things are wrong with the Serialize Functions here
    buffer.Push(pshEntry);

    REQUIRE(buffer.Size() == 1);
    REQUIRE(buffer.Front().status == sts1cobcsw::edu::ProgramStatus::programRunning);
}
