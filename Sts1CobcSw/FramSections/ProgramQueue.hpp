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

#include <rodos-semaphore.h>


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

    static constexpr auto section = eduProgramQueueSection;

    [[nodiscard]] static constexpr auto FramCapacity() -> SizeType;
    [[nodiscard]] static constexpr auto CacheCapacity() -> SizeType;

    [[nodiscard]] static auto Size() -> SizeType;
    [[nodiscard]] static auto Empty() -> bool;
    [[nodiscard]] static auto Full() -> bool;


    static auto Get(IndexType index) -> T;
    //! @brief Pushes a value to the back of the program queue.
    //! If it is full, a debug message is printed.
    static auto PushBack(T const & t) -> void;

    static auto Clear() -> void;

private:
    static constexpr auto elementSize = fram::Size(serialSize<T>);
    static constexpr auto indexesSize = fram::Size(3 * totalSerialSize<SizeType>);

    static constexpr auto subsections =
        Subsections<section,
                    SubsectionInfo<"indexes", indexesSize>,
                    SubsectionInfo<"array", section.size - indexesSize>>{};

    static constexpr auto persistentIndexes =
        PersistentVariables<subsections.template Get<"indexes">(),
                            PersistentVariableInfo<"size", SizeType>>{};

    static constexpr auto framCapacity = subsections.template Get<"array">().size / elementSize;
    static constexpr auto spiTimeout = elementSize < 300U ? 1 * ms : value_of(elementSize) * 3 * us;

    static inline auto cache = etl::vector<T, nCachedElements>{};

    static auto LoadSize() -> void;
    static auto StoreSize() -> void;

    static inline auto size = 0U;
    static inline auto semaphore = RODOS::Semaphore{};

    static auto ReadElement(IndexType index) -> T;
    static auto WriteElement(IndexType index, T const & t) -> void;
};
}

#include <Sts1CobcSw/FramSections/ProgramQueue.ipp>  // IWYU pragma: keep
