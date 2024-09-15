#pragma once


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

#include <rodos/api/rodos-semaphore.h>

#include <etl/circular_buffer.h>
#include <etl/cyclic_value.h>

#include <cstddef>
#include <cstdint>
#include <span>


namespace sts1cobcsw
{
template<typename T, Section ringArraySection, std::size_t nCachedElements>
    requires(serialSize<T> > 0)
class RingArray
{
public:
    using ValueType = T;
    static constexpr auto section = ringArraySection;

    [[nodiscard]] static constexpr auto FramCapacity() -> std::size_t;
    [[nodiscard]] static constexpr auto CacheCapacity() -> std::size_t;
    [[nodiscard]] static auto Size() -> std::size_t;
    [[nodiscard]] static auto Get(std::size_t index) -> T;
    [[nodiscard]] static auto Front() -> T;
    [[nodiscard]] static auto Back() -> T;
    static auto Set(std::size_t index, T const & t) -> void;
    static auto PushBack(T const & t) -> void;


private:
    static constexpr auto elementSize = fram::Size(serialSize<T>);
    static constexpr auto indexesSize = fram::Size(3 * 2 * totalSerialSize<std::size_t>);
    static constexpr auto subsections =
        Subsections<section,
                    SubsectionInfo<"indexes", indexesSize>,
                    SubsectionInfo<"array", section.size - indexesSize>>{};
    static constexpr auto persistentIndexes =
        PersistentVariables<subsections.template Get<"indexes">(),
                            PersistentVariableInfo<"iBegin", std::size_t>,
                            PersistentVariableInfo<"iEnd", std::size_t>>{};
    // We reduce the FRAM capacity by one to distinguish between an empty and a full ring. See
    // PushBack() for details.
    static constexpr auto framCapacity = subsections.template Get<"array">().size / elementSize - 1;
    static constexpr auto spiTimeout = elementSize < 300U ? 1 * ms : value_of(elementSize) * 3 * us;


    using RingIndex = etl::cyclic_value<std::size_t, 0, framCapacity>;

    static RingIndex iEnd;
    static RingIndex iBegin;
    static etl::circular_buffer<SerialBuffer<T>, nCachedElements> cache;
    static RODOS::Semaphore semaphore;

    static auto LoadIndexes() -> void;
    static auto StoreIndexes() -> void;
    [[nodiscard]] static auto ComputeSize() -> std::size_t;
    [[nodiscard]] static auto ReadElement(RingIndex index) -> T;
    static auto WriteElement(RingIndex index, T const & t) -> void;
};
}


#include <Sts1CobcSw/Periphery/FramRingArray.ipp>  // IWYU pragma: keep
