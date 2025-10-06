#include <Sts1CobcSw/RfProtocols/CcsdsFileDeliveryProtocol.hpp>

#include <etl/utility.h>

#include <cstdint>
#include <span>


namespace sts1cobcsw
{
// The order of the segments is not important and for higher performance also not preserved
auto UpdateMissingFileData(etl::ivector<SegmentRequest> * missingFileData,
                           FileDataPdu const & newFileDataPdu) -> void
{
    auto newFileDataEndOffset =
        static_cast<std::uint32_t>(newFileDataPdu.offset_ + newFileDataPdu.fileData_.size());
    for(auto && segment : *missingFileData)
    {
        if(segment.startOffset < newFileDataPdu.offset_ and newFileDataEndOffset < segment.endOffset
           and not missingFileData->full())
        {
            // Split the segment, appending the second half to the end -> order is not preserved
            auto oldEndOffset = segment.endOffset;
            segment.endOffset = newFileDataPdu.offset_;
            missingFileData->push_back(
                SegmentRequest{.startOffset = newFileDataEndOffset, .endOffset = oldEndOffset});
            return;
        }
        if(segment.startOffset == newFileDataPdu.offset_
           or segment.endOffset == newFileDataEndOffset)
        {
            if(segment.startOffset == newFileDataPdu.offset_)
            {
                segment.startOffset += newFileDataPdu.fileData_.size();
            }
            else if(segment.endOffset == newFileDataEndOffset)
            {
                segment.endOffset -= newFileDataPdu.fileData_.size();
            }
            if(segment.startOffset >= segment.endOffset)
            {
                // Use swap-and-pop to remove the segment -> order is not preserved
                segment = missingFileData->back();
                missingFileData->pop_back();
            }
            return;
        }
    }
}
}
