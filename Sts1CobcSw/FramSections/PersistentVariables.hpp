#pragma once


#include <Sts1CobcSw/FramSections/PersistentVariableInfo.hpp>
#include <Sts1CobcSw/FramSections/Section.hpp>
#include <Sts1CobcSw/FramSections/Subsections.hpp>
#include <Sts1CobcSw/Periphery/Fram.hpp>

#include <rodos/api/rodos-semaphore.h>
#include <rodos/api/timemodel.h>

#include <optional>

#include "Sts1CobcSw/Utility/TimeTypes.hpp"


namespace sts1cobcsw
{
template<Section persistentVariablesSection, APersistentVariableInfo... PersistentVariableInfos>
    requires(sizeof...(PersistentVariableInfos) > 0)
class PersistentVariables
{
public:
    static constexpr auto section = persistentVariablesSection;

    template<StringLiteral name>
    using ValueType = std::tuple_element_t<
        Subsections<section, PersistentVariableInfos...>::template Index<name>(),
        std::tuple<typename PersistentVariableInfos::ValueType...>>;

    template<StringLiteral name>
    [[nodiscard]] static auto Load() -> ValueType<name>;
    template<StringLiteral name>
    [[nodiscard]] static auto Store(ValueType<name> const & value);
    template<StringLiteral name>
    [[nodiscard]] static auto Increment();


private:
    template<StringLiteral name>
    [[nodiscard]] static auto ReadFromFram() -> std::array<ValueType<name>, 3>;
    template<StringLiteral name>
    [[nodiscard]] static auto WriteToFram(ValueType<name> const & value);
    template<StringLiteral name>
    [[nodiscard]] static auto ReadFromCache() -> std::array<ValueType<name>, 3>;
    template<StringLiteral name>
    [[nodiscard]] static auto WriteToCache(ValueType<name> const & value);

    static std::tuple<typename PersistentVariableInfos::ValueType...> cache0;
    static std::tuple<typename PersistentVariableInfos::ValueType...> cache1;
    static std::tuple<typename PersistentVariableInfos::ValueType...> cache2;

    static constexpr auto subsections = Subsections<section,
                                                    SubsectionInfo<"0", section.size / 3>,
                                                    SubsectionInfo<"1", section.size / 3>,
                                                    SubsectionInfo<"2", section.size / 3>>{};
    static constexpr auto variables0 =
        Subsections<subsections.template Get<"0">(), PersistentVariableInfos...>();
    static constexpr auto variables1 =
        Subsections<subsections.template Get<"1">(), PersistentVariableInfos...>();
    static constexpr auto variables2 =
        Subsections<subsections.template Get<"2">(), PersistentVariableInfos...>();

    // With a baud rate of 48 MHz we can read 6000 bytes in 1 ms, which should be more than enough
    static constexpr auto spiTimeout = 1 * Duration(RODOS::MILLISECONDS);

    static RODOS::Semaphore semaphore;
};
}


#include <Sts1CobcSw/FramSections/PersistentVariables.ipp>  // IWYU pragma: keep
