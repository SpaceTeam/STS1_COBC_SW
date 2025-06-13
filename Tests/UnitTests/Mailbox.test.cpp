#include <Tests/CatchRodos/TestMacros.hpp>
#include <Tests/Utility/Stringification.hpp>  // IWYU pragma: keep

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
    CHECK(getResult.error() == ErrorCode::empty);

    auto putResult = mailbox.Put(1);
    CHECK(putResult.has_value());

    putResult = mailbox.Put(1);
    CHECK(putResult.has_error());
    CHECK(putResult.error() == ErrorCode::full);
    CHECK(mailbox.IsEmpty() == false);
    CHECK(mailbox.IsFull());

    mailbox.Overwrite(2);

    auto peekResult = mailbox.Peek();
    CHECK(peekResult.has_value());
    CHECK(peekResult.value() == 2);
    CHECK(mailbox.IsEmpty() == false);
    CHECK(mailbox.IsFull());

    getResult = mailbox.Get();
    CHECK(getResult.has_value());
    CHECK(getResult.value() == 2);
    CHECK(mailbox.IsEmpty());
    CHECK(mailbox.IsFull() == false);

    auto suspendResult = mailbox.SuspendUntilEmpty(1 * ms);
    CHECK(suspendResult.has_value());

    suspendResult = mailbox.SuspendUntilFull(1 * ms);
    CHECK(suspendResult.has_error());
    CHECK(suspendResult.error() == ErrorCode::timeout);

    putResult = mailbox.Put(1);
    CHECK(putResult.has_value());

    suspendResult = mailbox.SuspendUntilEmpty(1 * ms);
    CHECK(suspendResult.has_error());
    CHECK(suspendResult.error() == ErrorCode::timeout);

    suspendResult = mailbox.SuspendUntilFull(1 * ms);
    CHECK(suspendResult.has_value());
}
}
