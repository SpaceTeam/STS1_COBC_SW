#pragma once


#include <Sts1CobcSw/Outcome/Outcome.hpp>
#include <Sts1CobcSw/RfProtocols/Configuration.hpp>
#include <Sts1CobcSw/RfProtocols/Payload.hpp>
#include <Sts1CobcSw/RfProtocols/ProtocolDataUnitHeader.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>

#include <etl/utility.h>
#include <etl/vector.h>

#include <cstdint>
#include <span>


namespace sts1cobcsw
{
namespace tc
{
struct ProtocolDataUnit
{
    using Header = ProtocolDataUnitHeader;

    Header header;
    etl::vector<Byte, tc::maxPduDataLength> dataField;
};
}


class FileDataPdu : public Payload
{
public:
    std::uint32_t offset = 0;
    std::span<Byte const> fileData;


private:
    auto DoAddTo(etl::ivector<Byte> * dataField) const -> void override;
    [[nodiscard]] auto DoSize() const -> std::uint16_t override;
};


[[nodiscard]] auto ParseAsProtocolDataUnit(std::span<Byte const> buffer)
    -> Result<tc::ProtocolDataUnit>;
[[nodiscard]] auto ParseAsFileDataPdu(std::span<Byte const> buffer) -> Result<FileDataPdu>;
}
