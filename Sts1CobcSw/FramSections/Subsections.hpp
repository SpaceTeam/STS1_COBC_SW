#pragma once


#include <Sts1CobcSw/FramSections/Section.hpp>
#include <Sts1CobcSw/FramSections/SubsectionInfo.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <numeric>


namespace sts1cobcsw
{
template<Section parentSection, SubsectionInfoLike... SubsectionInfos>
    requires(sizeof...(SubsectionInfos) > 0 and containsNoDuplicateNames<SubsectionInfos...>)
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
        -> std::array<fram::Address, sizeof...(SubsectionInfos)>;
    [[nodiscard]] static constexpr auto ComputeEnds()
        -> std::array<fram::Address, sizeof...(SubsectionInfos)>;

    // NOLINTBEGIN(readability-identifier-naming): clang-tidy doesn't recognize these as private
    // members but as constexpr variables
    static constexpr auto sizes_ = std::array{SubsectionInfos::size...};
    static constexpr auto begins_ = ComputeBegins();
    static constexpr auto ends_ = ComputeEnds();
    // NOLINTEND(readability-identifier-naming)
};
}


#include <Sts1CobcSw/FramSections/Subsections.ipp>
