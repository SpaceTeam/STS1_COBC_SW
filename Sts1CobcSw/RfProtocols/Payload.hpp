#pragma once


#include <Sts1CobcSw/Outcome/Outcome.hpp>
#include <Sts1CobcSw/Serial/Byte.hpp>

#include <etl/vector.h>

#include <cstdint>


namespace sts1cobcsw
{
class Payload
{
public:
    Payload() = default;
    virtual ~Payload() = default;


protected:
    Payload(Payload const &) = default;
    Payload(Payload &&) = default;
    auto operator=(Payload const &) -> Payload & = default;
    auto operator=(Payload &&) -> Payload & = default;


public:
    auto AddTo(etl::ivector<Byte> * dataField) const -> Result<void>
    {
        if(dataField->available() < DoSize())
        {
            // TODO: Think about what error to return here
            return ErrorCode::tooLarge;
        }
        DoAddTo(dataField);
        return outcome_v2::success();
    }


    [[nodiscard]] auto Size() const -> std::uint16_t
    {
        return DoSize();
    }


private:
    // Precondition: dataField->available() >= Size()
    virtual auto DoAddTo(etl::ivector<Byte> * dataField) const -> void = 0;
    [[nodiscard]] virtual auto DoSize() const -> std::uint16_t = 0;
};
}
