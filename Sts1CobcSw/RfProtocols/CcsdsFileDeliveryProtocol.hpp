#pragma once


#include <Sts1CobcSw/RfProtocols/ProtocolDataUnits.hpp>

#include <etl/vector.h>


namespace sts1cobcsw
{
auto UpdateMissingFileData(etl::ivector<SegmentRequest> * missingFileData,
                           FileDataPdu const & newFileDataPdu) -> void;
}
