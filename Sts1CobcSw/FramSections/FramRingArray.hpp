#pragma once


#include <Sts1CobcSw/Fram/Fram.hpp>
#include <Sts1CobcSw/FramSections/PersistentVariableInfo.hpp>
#include <Sts1CobcSw/FramSections/PersistentVariables.hpp>
#include <Sts1CobcSw/FramSections/Section.hpp>
#include <Sts1CobcSw/FramSections/SubsectionInfo.hpp>
#include <Sts1CobcSw/FramSections/Subsections.hpp>
#include <Sts1CobcSw/Serial/Serial.hpp>
#include <Sts1CobcSw/Utility/Span.hpp>
#include <Sts1CobcSw/Vocabulary/Time.hpp>

#include <rodos/api/rodos-semaphore.h>

#include <etl/circular_buffer.h>
#include <etl/cyclic_value.h>

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <span>


namespace sts1cobcsw
{
// I don't want to repeat strong::underlying_type_t<fram::Size> as the type of the template
// parameter nCachedElements of FramRingArray every time so I assert that it's uint32_t
static_assert(std::is_same_v<strong::underlying_type_t<fram::Size>, std::uint32_t>);


template<typename T, Section framRingArraySection, std::uint32_t nCachedElements>
    requires(serialSize<T> > 0)
class FramRingArray
{
public:
    using ValueType = T;
    using IndexType = strong::underlying_type_t<fram::Size>;
    using SizeType = strong::underlying_type_t<fram::Size>;

    static constexpr auto section = framRingArraySection;

    [[nodiscard]] static constexpr auto FramCapacity() -> SizeType;
    [[nodiscard]] static constexpr auto CacheCapacity() -> SizeType;
    [[nodiscard]] static auto Size() -> SizeType;
    // Return the last element if index >= size and T{} if the ring is empty
    [[nodiscard]] static auto Get(IndexType index) -> T;
    // Return T{} if the ring is empty
    [[nodiscard]] static auto Front() -> T;
    // Return T{} if the ring is empty
    [[nodiscard]] static auto Back() -> T;
    // Do nothing if index >= size
    static auto Set(IndexType index, T const & t) -> void;
    static auto PushBack(T const & t) -> void;
    // Find and replace the first element that satisfies the predicate. Do nothing if no such
    // element is found.
    static auto FindAndReplace(std::predicate<T> auto predicate, T const & newData) -> void;


private:
    static constexpr auto elementSize = fram::Size(serialSize<T>);
    static constexpr auto indexesSize = fram::Size(3 * 2 * totalSerialSize<IndexType>);
    static constexpr auto subsections =
        Subsections<section,
                    SubsectionInfo<"indexes", indexesSize>,
                    SubsectionInfo<"array", section.size - indexesSize>>{};
    static constexpr auto persistentIndexes =
        PersistentVariables<subsections.template Get<"indexes">(),
                            PersistentVariableInfo<"iBegin", IndexType>,
                            PersistentVariableInfo<"iEnd", IndexType>>{};
    // We reduce the FRAM capacity by one to distinguish between an empty and a full ring. See
    // PushBack() for details.
    static constexpr auto framCapacity = subsections.template Get<"array">().size / elementSize - 1;
    static constexpr auto spiTimeout = elementSize < 300U ? 1 * ms : value_of(elementSize) * 3 * us;
    // We use big endian because UInt<> doesn't support little endian
    static constexpr auto endianness = std::endian::big;

    using RingIndex = etl::cyclic_value<IndexType, 0, framCapacity>;
    struct Indexes
    {
        RingIndex iBegin;
        RingIndex iEnd;
    };

    static inline auto cache = etl::circular_buffer<SerialBuffer<T>, nCachedElements>{};
    static inline auto semaphore = RODOS::Semaphore{};

    [[nodiscard]] static auto FramArraySize(Indexes const & indexes) -> SizeType;
    [[nodiscard]] static auto GetFromCache(IndexType index) -> T;
    [[nodiscard]] static auto GetFromFram(IndexType index, Indexes const & indexes) -> T;
    static auto SetInCache(IndexType index, T const & t) -> void;
    static auto SetInFramAndCache(IndexType index, T const & t, Indexes const & indexes) -> void;
    [[nodiscard]] static auto LoadIndexes() -> Indexes;
    // NOLINTNEXTLINE(*unnecessary-value-param)
    [[nodiscard]] static auto LoadElement(RingIndex index) -> T;
    static auto StoreIndexes(Indexes const & indexes) -> void;
    // NOLINTNEXTLINE(*unnecessary-value-param)
    static auto StoreElement(RingIndex index, T const & t) -> void;
};
}


#include <Sts1CobcSw/FramSections/FramRingArray.ipp>  // IWYU pragma: keep
