#pragma once


#include <Sts1CobcSw/FramSections/Subsections.hpp>


namespace sts1cobcsw
{
template<Section parentSection, SubsectionInfoLike... SubsectionInfos>
    requires(sizeof...(SubsectionInfos) > 0 and containsNoDuplicateNames<SubsectionInfos...>)
template<StringLiteral subsectionName>
constexpr auto Subsections<parentSection, SubsectionInfos...>::Index() -> std::size_t
{
    constexpr auto hasSameName = std::array{subsectionName == SubsectionInfos::name...};
    constexpr auto count = std::accumulate(hasSameName.begin(), hasSameName.end(), 0);
    static_assert(count == 1, "There is no subsection with this name");
    auto iterator = std::find(hasSameName.begin(), hasSameName.end(), true);
    return static_cast<std::size_t>(std::distance(hasSameName.begin(), iterator));
}


template<Section parentSection, SubsectionInfoLike... SubsectionInfos>
    requires(sizeof...(SubsectionInfos) > 0 and containsNoDuplicateNames<SubsectionInfos...>)
constexpr auto Subsections<parentSection, SubsectionInfos...>::ComputeBegins()
    -> std::array<fram::Address, sizeof...(SubsectionInfos)>
{
    auto begins = std::array<fram::Address, sizeof...(SubsectionInfos)>{};
    for(std::size_t i = 0; i < sizeof...(SubsectionInfos); ++i)
    {
        begins[i] = std::accumulate(sizes_.begin(), sizes_.begin() + i, parentSection.begin);
    }
    return begins;
}


template<Section parentSection, SubsectionInfoLike... SubsectionInfos>
    requires(sizeof...(SubsectionInfos) > 0 and containsNoDuplicateNames<SubsectionInfos...>)
constexpr auto Subsections<parentSection, SubsectionInfos...>::ComputeEnds()
    -> std::array<fram::Address, sizeof...(SubsectionInfos)>
{
    {
        constexpr auto ends = []()
        {
            auto addresses = decltype(begins_){};
            for(std::size_t i = 0; i < sizeof...(SubsectionInfos); ++i)
            {
                addresses[i] = begins_[i] + sizes_[i];
            }
            return addresses;
        }();
        static_assert(ends.back() <= parentSection.end,
                      "The subsections do not fit into the parent section");
        return ends;
    }
}
}
