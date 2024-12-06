#pragma once



#include <Sts1CobcSw/FramSections/FramLayout.hpp>
#include <Sts1CobcSw/FramSections/PersistentVariableInfo.hpp>
#include <Sts1CobcSw/FramSections/PersistentVariables.hpp>
#include <Sts1CobcSw/FramSections/Section.hpp>
#include <Sts1CobcSw/FramSections/SubsectionInfo.hpp>
#include <Sts1CobcSw/FramSections/Subsections.hpp>
#include <Sts1CobcSw/Periphery/Fram.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>
#include <Sts1CobcSw/Utility/Span.hpp>
#include <Sts1CobcSw/Utility/Time.hpp>
#include <Sts1CobcSw/Utility/TimeTypes.hpp>




// TODO:
// Methods needed (used through the program) :
// - empty(),
// - full()
// - clear()
// - push_back()
// = []operator

namespace sts1cobcsw
{
template<typename T, Section eduProgramQueueSection, std::size_t nCachedElements>
    requires(serialSize<T> > 0)
class ProgramQueue
{
public:
    using ValueType = T;
    using IndexType = std::size_t;
    using SizeType = std::size_t;

    [[nodiscard]] static auto Empty() -> bool;
    [[nodiscard]] static auto Full() -> bool;
    [[nodiscard]] static auto Clear() -> bool;
    [[nodiscard]] static auto PushBack(T const & t) -> bool;

    [[nodiscard]] static auto Size() -> SizeType;

private:
    static constexpr auto elementSize = fram::Size(serialSize<T>);
    static constexpr auto framCapacity = eduProgramQueueSection.size / elementSize;

    static constexpr auto spiTimeout = elementSize < 300U ? 1 * ms : value_of(elementSize) * 3 * us;

    static inline auto iBegin = 0u;
    static inline auto size = 0u;

    [[nodiscard]] static auto FramCapacity() -> SizeType;

};
}

#include <Sts1CobcSw/FramSections/ProgramQueue.ipp>  // IWYU pragma: keep
