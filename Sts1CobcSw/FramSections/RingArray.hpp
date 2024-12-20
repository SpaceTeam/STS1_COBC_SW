#pragma once


#include <Sts1CobcSw/FramSections/PersistentVariableInfo.hpp>
#include <Sts1CobcSw/FramSections/PersistentVariables.hpp>
#include <Sts1CobcSw/FramSections/Section.hpp>
#include <Sts1CobcSw/FramSections/SubsectionInfo.hpp>
#include <Sts1CobcSw/FramSections/Subsections.hpp>
#include <Sts1CobcSw/Periphery/Fram.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>
#include <Sts1CobcSw/Utility/RodosTime.hpp>
#include <Sts1CobcSw/Utility/Span.hpp>

#include <rodos/api/rodos-semaphore.h>

#include <etl/circular_buffer.h>
#include <etl/cyclic_value.h>

#include <concepts>
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
    using IndexType = std::size_t;
    using SizeType = std::size_t;

    static constexpr auto section = ringArraySection;

    [[nodiscard]] static constexpr auto FramCapacity() -> SizeType;
    [[nodiscard]] static constexpr auto CacheCapacity() -> SizeType;
    [[nodiscard]] static auto Size() -> SizeType;
    [[nodiscard]] static auto Get(IndexType index) -> T;
    [[nodiscard]] static auto Front() -> T;
    [[nodiscard]] static auto Back() -> T;
    static auto Set(IndexType index, T const & t) -> void;
    static auto PushBack(T const & t) -> void;
    // TODO: Test this function
    static auto FindAndReplace(std::predicate<T> auto predicate, T const & newData) -> void;


private:
    using RingIndexType = strong::underlying_type_t<fram::Size>;

    static constexpr auto elementSize = fram::Size(serialSize<T>);
    static constexpr auto indexesSize = fram::Size(3 * 2 * totalSerialSize<RingIndexType>);
    static constexpr auto subsections =
        Subsections<section,
                    SubsectionInfo<"indexes", indexesSize>,
                    SubsectionInfo<"array", section.size - indexesSize>>{};
    static constexpr auto persistentIndexes =
        PersistentVariables<subsections.template Get<"indexes">(),
                            PersistentVariableInfo<"iBegin", RingIndexType>,
                            PersistentVariableInfo<"iEnd", RingIndexType>>{};
    // We reduce the FRAM capacity by one to distinguish between an empty and a full ring. See
    // PushBack() for details.
    static constexpr auto framCapacity = subsections.template Get<"array">().size / elementSize - 1;
    static constexpr auto spiTimeout = elementSize < 300U ? 1 * ms : value_of(elementSize) * 3 * us;

    using RingIndex = etl::cyclic_value<RingIndexType, 0, framCapacity>;

    static inline auto iBegin = RingIndex{};
    static inline auto iEnd = RingIndex{};
    static inline auto cache = etl::circular_buffer<SerialBuffer<T>, nCachedElements>{};
    static inline auto semaphore = RODOS::Semaphore{};

    [[nodiscard]] static auto DoSize() -> SizeType;
    [[nodiscard]] static auto DoGet(IndexType index) -> T;
    static auto DoSet(IndexType index, T const & t) -> void;
    static auto LoadIndexes() -> void;
    static auto StoreIndexes() -> void;
    [[nodiscard]] static auto FramSize() -> SizeType;
    [[nodiscard]] static auto ReadElement(RingIndex index) -> T;
    static auto WriteElement(RingIndex index, T const & t) -> void;
};
}


#include <Sts1CobcSw/FramSections/RingArray.ipp>  // IWYU pragma: keep
