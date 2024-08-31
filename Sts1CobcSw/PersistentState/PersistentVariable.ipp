#pragma once


#include <Sts1CobcSw/PersistentState/PersistentVariable.hpp>
#include <Sts1CobcSw/Utility/Span.hpp>

#include <span>


namespace sts1cobcsw
{
template<typename T, fram::Address address, fram::Size offset>
PersistentVariable<T, address, offset>::PersistentVariable() = default;


template<typename T, fram::Address address, fram::Size offset>
auto PersistentVariable<T, address, offset>::Store(T const & t) -> void
{
    fram::WriteTo(address, std::as_bytes(Span(t)), 3);
    fram::WriteTo(address + offset, std::as_bytes(Span(t)), 3);
    fram::WriteTo(address + offset + offset, std::as_bytes(Span(t)), 3);
    t1_ = t;
    t2_ = t;
    t3_ = t;
}


template<typename T, fram::Address address, fram::Size offset>
auto PersistentVariable<T, address, offset>::Load() -> T
{
    T t1;  // NOLINT(readability-identifier-length, *member-init)
    T t2;  // NOLINT(readability-identifier-length, *member-init)
    T t3;  // NOLINT(readability-identifier-length, *member-init)
    fram::ReadFrom(address, std::as_writable_bytes(Span(&t1)), 3);
    fram::ReadFrom(address + offset, std::as_writable_bytes(Span(&t2)), 3);
    fram::ReadFrom(address + offset + offset, std::as_writable_bytes(Span(&t3)), 3);
    // FIXME: What is this supposed to do?
    // if(!t1 && !t2 && !t3)
    // {
    //     auto result = MajorityVote(t1_, t2_, t3_);
    //     if(t1_ != result)
    //     {
    //         t1_ = result;
    //     }
    //     if(t2_ != result)
    //     {
    //         t2_ = result;
    //     }
    //     if(t3_ != result)
    //     {
    //         t3_ = result;
    //     }
    //     return result;
    // }
    auto result = MajorityVote(t1, t2, t3);
    if(t1 != result)
    {
        fram::WriteTo(address, std::as_bytes(Span(result)), 3);
    }
    if(t2 != result)
    {
        fram::WriteTo(address + offset, std::as_bytes(Span(result)), 3);
    }
    if(t3 != result)
    {
        fram::WriteTo(address + offset + offset, std::as_bytes(Span(result)), 3);
    }
    return result;
}


template<typename T, fram::Address address, fram::Size offset>
auto PersistentVariable<T, address, offset>::MajorityVote(T const & vote1,
                                                          T const & vote2,
                                                          T const & vote3) -> T
{
    if(vote1 == vote2 || vote1 == vote3)
    {
        return vote1;
    }
    if(vote2 == vote3)
    {
        return vote2;
    }
    return vote1;
}
}
