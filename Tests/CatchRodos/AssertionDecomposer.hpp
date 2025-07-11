#pragma once


#include <Tests/CatchRodos/Assertion.hpp>
#include <Tests/CatchRodos/TestReporter.hpp>
#include <Tests/CatchRodos/Vocabulary.hpp>

#include <utility>


namespace sts1cobcsw
{
template<typename LeftValue>
struct LeftExpression
{
    LeftValue leftValue;  // NOLINT(*avoid-const-or-ref-data-members)
    Assertion assertion;


    template<typename RightValue>
    auto operator==(RightValue && rightValue) -> bool
    {
        assertion.op = "==";
        // NOLINTNEXTLINE(*array-to-pointer-decay,*no-array-decay)
        return Assert(leftValue == rightValue, std::forward<RightValue>(rightValue));
    }


    template<typename RightValue>
    auto operator!=(RightValue && rightValue) -> bool
    {
        assertion.op = "!=";
        // NOLINTNEXTLINE(*array-to-pointer-decay,*no-array-decay)
        return Assert(leftValue != rightValue, std::forward<RightValue>(rightValue));
    }


    template<typename RightValue>
    auto operator<(RightValue && rightValue) -> bool
    {
        assertion.op = "<";
        // NOLINTNEXTLINE(*array-to-pointer-decay,*no-array-decay)
        return Assert(leftValue < rightValue, std::forward<RightValue>(rightValue));
    }


    template<typename RightValue>
    auto operator<=(RightValue && rightValue) -> bool
    {
        assertion.op = "<=";
        // NOLINTNEXTLINE(*array-to-pointer-decay,*no-array-decay)
        return Assert(leftValue <= rightValue, std::forward<RightValue>(rightValue));
    }


    template<typename RightValue>
    auto operator>=(RightValue && rightValue) -> bool
    {
        assertion.op = ">=";
        // NOLINTNEXTLINE(*array-to-pointer-decay,*no-array-decay)
        return Assert(leftValue >= rightValue, std::forward<RightValue>(rightValue));
    }


    template<typename RightValue>
    auto operator>(RightValue && rightValue) -> bool
    {
        assertion.op = ">";
        // NOLINTNEXTLINE(*array-to-pointer-decay,*no-array-decay)
        return Assert(leftValue > rightValue, std::forward<RightValue>(rightValue));
    }


private:
    template<typename RightValue>
    auto Assert(bool condition, RightValue && rightValue) -> bool
    {
        if(condition)
        {
            TestReporter::GetInstance().AddPassedAssertion();
            return true;
        }
        TestReporter::GetInstance().AddFailedAssertion(
            assertion, std::forward<LeftValue>(leftValue), std::forward<RightValue>(rightValue));
        return false;
    }
};


struct AssertionDecomposer
{
    Assertion assertion;

    template<typename T>
    auto operator<(T && leftValue) -> LeftExpression<T>
    {
        return LeftExpression<T>{std::forward<T>(leftValue), assertion};
    }
};
}
