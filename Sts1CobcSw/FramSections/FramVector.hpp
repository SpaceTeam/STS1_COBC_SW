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
#include <Sts1CobcSw/Vocabulary/Time.hpp>

#include <rodos/api/rodos-semaphore.h>


namespace sts1cobcsw
{
// I don't want to repeat strong::underlying_type_t<fram::Size> as the type of the template
// parameter nCachedElements of FramRingArray every time so I assert that it's uint32_t
static_assert(std::is_same_v<strong::underlying_type_t<fram::Size>, std::uint32_t>);


template<typename T, Section framVectorSection, std::uint32_t nCachedElements>
    requires(serialSize<T> > 0)
class FramVector
{
public:
    using ValueType = T;
    using IndexType = strong::underlying_type_t<fram::Size>;
    using SizeType = strong::underlying_type_t<fram::Size>;

    static constexpr auto section = framVectorSection;

    [[nodiscard]] static constexpr auto FramCapacity() -> SizeType;
    [[nodiscard]] static constexpr auto CacheCapacity() -> SizeType;
    [[nodiscard]] static auto Size() -> SizeType;
    [[nodiscard]] static auto IsEmpty() -> bool;
    [[nodiscard]] static auto IsFull() -> bool;
    // Return the last element if index >= size and T{} if the vector is empty
    [[nodiscard]] static auto Get(IndexType index) -> T;
    // Do nothing if vector is full
    static auto PushBack(T const & t) -> void;
    static auto Clear() -> void;


private:
    static constexpr auto elementSize = fram::Size(serialSize<T>);
    static constexpr auto metadataSize = fram::Size(3 * totalSerialSize<SizeType>);
    static constexpr auto subsections =
        Subsections<section,
                    SubsectionInfo<"metadata", metadataSize>,
                    SubsectionInfo<"array", section.size - metadataSize>>{};
    static constexpr auto persistentMetadata =
        PersistentVariables<subsections.template Get<"metadata">(),
                            PersistentVariableInfo<"size", SizeType>>{};
    static constexpr auto framCapacity = subsections.template Get<"array">().size / elementSize;
    static constexpr auto spiTimeout = elementSize < 300U ? 1 * ms : value_of(elementSize) * 3 * us;

    static inline auto cache = etl::vector<SerialBuffer<T>, nCachedElements>{};
    static inline auto semaphore = RODOS::Semaphore{};

    static auto DoSize() -> SizeType;
    static auto LoadSize() -> SizeType;
    static auto LoadElement(IndexType index) -> T;
    static auto StoreSize(SizeType size) -> void;
    static auto StoreElement(IndexType index, T const & t) -> void;
};
}

#include <Sts1CobcSw/FramSections/FramVector.ipp>  // IWYU pragma: keep
