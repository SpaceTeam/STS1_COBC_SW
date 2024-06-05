
#include <Sts1CobcSw/Periphery/Fram.hpp>
#include <Sts1CobcSw/PersistentState/PersistentVariable.hpp>

namespace sts1cobcsw
{
template<typename T, std::uint32_t address, std::uint32_t offset>
PersistentVariable<T, address, offset>::PersistentVariable() = default;

template<typename T, std::uint32_t address, std::uint32_t offset>
auto PersistentVariable<T, address, offset>::Store(T t) -> void
{
    fram::WriteTo(address, t, sizeof(T), 3);
    fram::WriteTo(address + offset, t, sizeof(T), 3);
    fram::WriteTo(address + 2 * offset, t, sizeof(T), 3);
    t1_ = t;
    t2_ = t;
    t3_ = t;
}
template<typename T, std::uint32_t address, std::uint32_t offset>
auto PersistentVariable<T, address, offset>::Load() -> T
{
    T t1;
    T t2;
    T t3;
    fram::ReadFrom(address, t1, sizeof(T), 3);
    fram::ReadFrom(address + offset, t2, sizeof(T), 3);
    fram::ReadFrom(address + 2 * offset, t3, sizeof(T), 3);
    if(!t1 && !t2 && !t3)
    {
        auto result = MajorityVote(t1_, t2_, t3_);
        if(t1_ != result)
        {
            t1_ = result;
        }
        if(t2_ != result)
        {
            t2_ = result;
        }
        if(t3_ != result)
        {
            t3_ = result;
        }
        return result;
    }
    auto result = MajorityVote(t1, t2, t3);
    if(t1 != result)
    {
        fram::WriteTo(address, result, sizeof(T), 3);
    }
    if(t2 != result)
    {
        fram::WriteTo(address + offset, result, sizeof(T), 3);
    }
    if(t3 != result)
    {
        fram::WriteTo(address + 2 * offset, result, sizeof(T), 3);
    }
    return result;
}
template<typename T, std::uint32_t address, std::uint32_t offset>
auto PersistentVariable<T, address, offset>::MajorityVote(T vote1, T vote2, T vote3) -> T
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
