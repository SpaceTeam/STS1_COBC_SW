#pragma once


#include <Sts1CobcSw/Serial/Serial.hpp>

#include <array>
#include <type_traits>


namespace sts1cobcsw
{
// TODO: Think about adding a tag. The problem is that I cannot add a defaulted template parameter
// after the pack.
template<typename T, T... validIdValues>
    requires(sizeof...(validIdValues) > 0)
struct Id
{
    using ValueType = T;

    static constexpr auto validValues = std::array{validIdValues...};
    // Must be public for Id<> to be a structural type and therefore usable as an NTTP
    T valueIsAnImplementationDetail = {};

    constexpr Id() = default;
    explicit constexpr Id(T t);

    [[nodiscard]] constexpr auto Value() const -> T;
    friend constexpr auto operator==(Id const &, Id const &) -> bool = default;
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


template<AnId Id>
inline constexpr std::size_t serialSize<Id> = totalSerialSize<typename Id::ValueType>;


template<AnId Id>
[[nodiscard]] constexpr auto IsValid(Id const & id) -> bool;

template<AnId Id, typename Id::ValueType t>
    requires(IsValid(Id(t)))
[[nodiscard]] constexpr auto Make() -> Id;

template<std::endian endianness, AnId Id>
auto SerializeTo(void * destination, Id const & id) -> void *;
template<std::endian endianness, AnId Id>
auto DeserializeFrom(void const * source, Id * id) -> void const *;
}


#include <Sts1CobcSw/RfProtocols/Id.ipp>  // IWYU pragma: keep
