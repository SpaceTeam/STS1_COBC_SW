#include <concepts>
#include <type_traits>


namespace sts1cobcsw::serialize
{
template<typename T>
concept TriviallySerializable = std::is_arithmetic<T>::value or std::is_enum<T>::value;
}