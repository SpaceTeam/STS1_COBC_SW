#include <Sts1CobcSw/SingleBuffer/SingleBuffer.hpp>

#include <Tests/CatchRodos/TestMacros.hpp>

#include <Sts1CobcSw/Outcome/Outcome.hpp>
#include <Sts1CobcSw/Vocabulary/Time.hpp>

#include <strong_type/difference.hpp>
#include <strong_type/type.hpp>

#include <utility>


namespace sts1cobcsw
{

TEST_CASE("SingleBuffer")
{
    auto buffer = SingleBuffer<int>{};

    CHECK(buffer.IsEmpty());

    CHECK(buffer.IsFull() == false);

    auto getResult = buffer.Get();
    CHECK(getResult.has_error());

    auto putResult = buffer.Put(1);
    CHECK(putResult.has_error() == false);

    putResult = buffer.Put(1);
    CHECK(putResult.has_error());

    CHECK(buffer.IsEmpty() == false);

    CHECK(buffer.IsFull());

    getResult = buffer.Get();
    CHECK(getResult.has_error() == false);

    auto suspendResult = buffer.SuspendUntilEmpty(1 * ms);
    CHECK(suspendResult.has_error() == false);

    suspendResult = buffer.SuspendUntilFull(1 * ms);
    CHECK(suspendResult.has_error());
    CHECK(suspendResult.error() == ErrorCode::timeout);

    putResult = buffer.Put(1);
    CHECK(putResult.has_error() == false);

    suspendResult = buffer.SuspendUntilEmpty(1 * ms);
    CHECK(suspendResult.has_error());
    CHECK(suspendResult.error() == ErrorCode::timeout);

    suspendResult = buffer.SuspendUntilFull(1 * ms);
    CHECK(suspendResult.has_error() == false);
}

}
