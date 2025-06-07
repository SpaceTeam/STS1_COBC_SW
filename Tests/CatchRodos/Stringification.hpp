#pragma once


#include <Tests/CatchRodos/Vocabulary.hpp>

#include <etl/format_spec.h>
#include <etl/to_string.h>
#include <etl/vector.h>

#include <array>
#include <cstddef>
#include <cstdint>
// IWYU pragma: no_include <version>


namespace sts1cobcsw
{
// Default implementation of Append() prints "??". Overload this function to provide proper
// stringification for your types.
template<typename T>
    requires(not Number<T>)
inline auto Append(ValueString * string, [[maybe_unused]] T const & value) -> void
{
    // etl::string::append() automatically truncates the string if it exceeds the maximum length
    string->append("??");
}


inline auto Append(ValueString * string, gsl::czstring value) -> void
{
    string->append(value);
}


inline auto Append(ValueString * string, bool value) -> void
{
    string->append(value ? "true" : "false");
}


inline auto Append(ValueString * string, char value) -> void
{
    string->append(1, value);
}


inline auto Append(ValueString * string, SignedNumber auto value) -> void
{
    etl::to_string(value, *string, /*append=*/true);
}


inline auto Append(ValueString * string, UnsignedNumber auto value) -> void
{
    string->append("0x");
    etl::to_string(value, *string, etl::format_spec{}.hex(), /*append=*/true);
}


inline auto Append(ValueString * string, std::byte value) -> void
{
    string->append("0x");
    etl::to_string(static_cast<unsigned char>(value),
                   *string,
                   etl::format_spec{}.hex().width(2).fill('0'),
                   /*append=*/true);
}


inline auto Append(ValueString * string, std::nullptr_t) -> void
{
    string->append("nullptr");
}


template<typename T>
inline auto Append(ValueString * string, T * value) -> void
{
    if(value == nullptr)
    {
        Append(string, nullptr);
    }
    else
    {
        string->append("0x");
        auto address = reinterpret_cast<std::uintptr_t>(value);  // NOLINT(*reinterpret-cast)
        etl::to_string(address,
                       *string,
                       etl::format_spec{}.hex().width(sizeof(void *) * 2).fill('0'),
                       /*append=*/true);
    }
}


template<typename T, std::size_t size>
inline auto Append(ValueString * string, std::array<T, size> const & value) -> void
{
    string->append("{");
    for(auto i = 0U; i < value.size(); ++i)
    {
        Append(string, value[i]);
        if(i < value.size() - 1)
        {
            string->append(", ");
        }
    }
    string->append("}");
}


template<typename T, std::size_t capacity>
inline auto Append(ValueString * string, etl::vector<T, capacity> const & value) -> void
{
    string->append("{");
    for(auto i = 0U; i < value.size(); ++i)
    {
        Append(string, value[i]);
        if(i < value.size() - 1)
        {
            string->append(", ");
        }
    }
    string->append("}");
}
}
