#pragma once


#include <Sts1CobcSw/RfProtocols/Vocabulary.hpp>


namespace sts1cobcsw
{
constexpr auto IsValid(Parameter::Id parameterId) -> bool
{
    switch(parameterId)
    {
        case Parameter::Id::rxDataRate:
        case Parameter::Id::txDataRate:
        case Parameter::Id::maxEduIdleDuration:
        case Parameter::Id::newEduResultIsAvailable:
            return true;
    }
    return false;
}
}
