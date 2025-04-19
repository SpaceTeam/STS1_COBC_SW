#pragma once


#include <Sts1CobcSw/RfProtocols/Id.hpp>

#include <rodos_no_using_namespace.h>


namespace sts1cobcsw
{
// I did not add an .ipp file because this class and its member functions are so simple
template<typename T, AnId Id>
class IdCounters
{
public:
    [[nodiscard]] auto Get(Id const & id) -> T
    {
        auto protector = RODOS::ScopeProtector(&semaphore_);  // NOLINT(*casting)
        return counters_[GetIndex(id)];
    }


    [[nodiscard]] auto PostIncrement(Id const & id) -> T
    {
        auto protector = RODOS::ScopeProtector(&semaphore_);  // NOLINT(*casting)
        return counters_[GetIndex(id)]++;
    }


private:
    std::array<T, Id::validValues.size()> counters_ = {};
    RODOS::Semaphore semaphore_;

    static constexpr auto GetIndex(Id const & id) -> std::size_t
    {
        auto it = std::ranges::find(Id::validValues, id.Value());  // NOLINT(*qualified-auto)
        return static_cast<std::size_t>(std::distance(Id::validValues.begin(), it));
    }
};
}
