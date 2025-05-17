#include <Tests/CatchRodos/Vocabulary.hpp>

#include <Sts1CobcSw/Outcome/Outcome.hpp>
#include <Sts1CobcSw/Vocabulary/Time.hpp>


namespace sts1cobcsw
{
// Stringification for CatchRodos
auto Append(ValueString * string, ErrorCode const & errorCode) -> void;
auto Append(ValueString * string, RealTime const & realTime) -> void;
}
