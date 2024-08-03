#pragma once


#include <Sts1CobcSw/Periphery/Section.hpp>
#include <Sts1CobcSw/Periphery/SubsectionInfo.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <numeric>


namespace sts1cobcsw::fram
{
// TODO: Test if Section parentSection, also works
template<auto parentSection, SubsectionInfoLike... SubsectionInfos>
    requires(isASection<decltype(parentSection)> and sizeof...(SubsectionInfos) > 0
             and containsNoDuplicateNames<SubsectionInfos...>)
class Subsections
{
public:
    constexpr Subsections() = default;

    template<StringLiteral subsectionName>
    [[nodiscard]] static constexpr auto Index() -> std::size_t;

    // I don't know how to explicitly specify the return type of this function so I have to keep it
    // in the .hpp file
    template<StringLiteral subsectionName>
    [[nodiscard]] static constexpr auto Get()
    {
        constexpr auto i = Index<subsectionName>();
        return Section<begins_[i], sizes_[i]>();
    }


private:
    [[nodiscard]] static constexpr auto ComputeBegins()
        -> std::array<Address, sizeof...(SubsectionInfos)>;
    [[nodiscard]] static constexpr auto ComputeEnds()
        -> std::array<Address, sizeof...(SubsectionInfos)>;

    // NOLINTBEGIN(readability-identifier-naming): clang-tidy doesn't recognize these as private
    // members but as constexpr variables
    static constexpr auto sizes_ = std::array{SubsectionInfos::size...};
    static constexpr auto begins_ = ComputeBegins();
    static constexpr auto ends_ = ComputeEnds();
    // NOLINTEND(readability-identifier-naming)
};
}


#include <Sts1CobcSw/Periphery/Subsections.ipp>
