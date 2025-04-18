#pragma once


#include <Sts1CobcSw/Outcome/Outcome.hpp>

#include <array>
#include <type_traits>


namespace sts1cobcsw
{
template<typename T, T... validIdValues>
class Id
{
public:
    static constexpr auto validValues = std::array{validIdValues...};

    template<T t>
        requires(((t == validIdValues) or ...))
    [[nodiscard]] static constexpr auto Make() -> Id;
    [[nodiscard]] static constexpr auto Make(T const & t) -> Result<Id>;

    [[nodiscard]] constexpr auto Value() const -> T;

    friend constexpr auto operator==(Id const &, Id const &) -> bool = default;

    // This must be public for Id<> to be a structural type and therefore usable as an NTTP
    T valueIsAnImplementationDetail;


private:
    template<T t>
        requires(((t == validIdValues) or ...))
    constexpr explicit Id(std::integral_constant<T, t> value);
    constexpr explicit Id(T const & t);
};
}


#include <Sts1CobcSw/RfProtocols/Id.ipp>  // IWYU pragma: keep
