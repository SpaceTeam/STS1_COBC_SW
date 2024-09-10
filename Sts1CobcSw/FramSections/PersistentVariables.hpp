#pragma once


#include <Sts1CobcSw/FramSections/PersistentVariableInfo.hpp>
#include <Sts1CobcSw/FramSections/Section.hpp>
#include <Sts1CobcSw/FramSections/Subsections.hpp>
#include <Sts1CobcSw/Periphery/Fram.hpp>

#include <rodos/api/rodos-semaphore.h>
#include <rodos/api/timemodel.h>

#include <optional>


namespace sts1cobcsw
{
template<Section parentSection0,
         Section parentSection1,
         Section parentSection2,
         APersistentVariableInfo... PersistentVariableInfos>
    requires(sizeof...(PersistentVariableInfos) > 0 && parentSection0.end <= parentSection1.begin
             && parentSection1.end <= parentSection2.begin)
class PersistentVariables
{
public:
    template<StringLiteral name>
    using ValueType = std::tuple_element_t<
        Subsections<parentSection0, PersistentVariableInfos...>::template Index<name>(),
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

    static constexpr auto subsections0 = Subsections<parentSection0, PersistentVariableInfos...>();
    static constexpr auto subsections1 = Subsections<parentSection1, PersistentVariableInfos...>();
    static constexpr auto subsections2 = Subsections<parentSection2, PersistentVariableInfos...>();

    // With a baud rate of 48 MHz we can read 6000 bytes in 1 ms, which should be more than enough
    static constexpr auto spiTimeout = 1 * RODOS::MILLISECONDS;

    static RODOS::Semaphore semaphore;
};
}


#include <Sts1CobcSw/FramSections/PersistentVariables.ipp>  // IWYU pragma: keep
