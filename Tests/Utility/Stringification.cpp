#include <Tests/CatchRodos/Stringification.hpp>
#include <Tests/Utility/Stringification.hpp>

#include <strong_type/type.hpp>

#include <utility>


namespace sts1cobcsw
{
auto Append(ValueString * string, ErrorCode errorCode) -> void
{
    Append(string, ToCZString(errorCode));
}


auto Append(ValueString * string, RealTime realTime) -> void
{
    Append(string, value_of(realTime));
    Append(string, " s");
}
}
