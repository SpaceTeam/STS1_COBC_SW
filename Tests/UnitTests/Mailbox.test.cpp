#include <Tests/CatchRodos/TestMacros.hpp>

#include <Sts1CobcSw/Mailbox/Mailbox.hpp>
#include <Sts1CobcSw/Outcome/Outcome.hpp>
#include <Sts1CobcSw/Vocabulary/Time.hpp>

#include <strong_type/difference.hpp>
#include <strong_type/type.hpp>

#include <utility>


namespace sts1cobcsw
{

TEST_CASE("Mailbox")
{
    auto mailbox = Mailbox<int>{};

    CHECK(mailbox.IsEmpty());

    CHECK(mailbox.IsFull() == false);

    auto getResult = mailbox.Get();
    CHECK(getResult.has_error());

    auto putResult = mailbox.Put(1);
    CHECK(putResult.has_error() == false);

    putResult = mailbox.Put(1);
    CHECK(putResult.has_error());

    CHECK(mailbox.IsEmpty() == false);

    CHECK(mailbox.IsFull());

    getResult = mailbox.Get();
    CHECK(getResult.has_error() == false);

    auto suspendResult = mailbox.SuspendUntilEmpty(1 * ms);
    CHECK(suspendResult.has_error() == false);

    suspendResult = mailbox.SuspendUntilFull(1 * ms);
    CHECK(suspendResult.has_error());
    CHECK(suspendResult.error() == ErrorCode::timeout);

    putResult = mailbox.Put(1);
    CHECK(putResult.has_error() == false);

    suspendResult = mailbox.SuspendUntilEmpty(1 * ms);
    CHECK(suspendResult.has_error());
    CHECK(suspendResult.error() == ErrorCode::timeout);

    suspendResult = mailbox.SuspendUntilFull(1 * ms);
    CHECK(suspendResult.has_error() == false);
}

}
