#pragma once


#include <Sts1CobcSw/Outcome/Outcome.hpp>

#include <array>
#include <type_traits>


namespace sts1cobcsw
{
template<typename T, T... validIdValues>
    requires(sizeof...(validIdValues) > 0)
class Id
{
public:
    using ValueType = T;

    static constexpr auto validValues = std::array{validIdValues...};

    explicit constexpr Id();

    template<T t>
        requires(((t == validIdValues) or ...))
    [[nodiscard]] static constexpr auto Make() -> Id;
    [[nodiscard]] static constexpr auto Make(T const & t) -> Result<Id>;

    [[nodiscard]] constexpr auto Value() const -> T;

    friend constexpr auto operator==(Id const &, Id const &) -> bool = default;

    // Must be public for Id<> to be a structural type and therefore usable as an NTTP
    T valueIsAnImplementationDetail;


private:
    template<T t>
        requires(((t == validIdValues) or ...))
    constexpr explicit Id(std::integral_constant<T, t> value);
    constexpr explicit Id(T const & t);
};


namespace internal
{
template<typename T>
inline constexpr auto isAnIdHelper = false;

template<typename T, T... validValues>
inline constexpr auto isAnIdHelper<Id<T, validValues...>> = true;
}


template<typename T>
inline constexpr bool isAnId = internal::isAnIdHelper<std::remove_cvref_t<T>>;


template<typename T>
concept AnId = isAnId<T>;
}


#include <Sts1CobcSw/RfProtocols/Id.ipp>  // IWYU pragma: keep
