#include <Sts1CobcSw/RfProtocols/Utility.hpp>


namespace sts1cobcsw
{
auto IncreaseSize(etl::ivector<Byte> * dataField, std::size_t sizeIncrease) -> std::size_t
{
    auto oldSize = dataField->size();
    dataField->resize(oldSize + sizeIncrease);
    return oldSize;
}
}
