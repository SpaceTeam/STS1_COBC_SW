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
    // TODO: Think about removing this (and the protected SMFs) because we never delete a child of
    // Payload through a pointer to Payload, because we don't use dynamic memory allocation.
    virtual ~Payload() = default;


protected:
    Payload(Payload const &) = default;
    Payload(Payload &&) = default;
    auto operator=(Payload const &) -> Payload & = default;
    auto operator=(Payload &&) -> Payload & = default;


public:
    // TODO: Think about renaming this to AddTo()
    auto WriteTo(etl::ivector<Byte> * dataField) const -> Result<void>
    {
        if(dataField->available() < DoSize())
        {
            // TODO: Think about what error to return here
            return ErrorCode::tooLarge;
        }
        DoWriteTo(dataField);
        return outcome_v2::success();
    }


    [[nodiscard]] auto Size() const -> std::uint16_t
    {
        return DoSize();
    }


private:
    // Precondition: dataField->available() >= Size()
    virtual auto DoWriteTo(etl::ivector<Byte> * dataField) const -> void = 0;
    [[nodiscard]] virtual auto DoSize() const -> std::uint16_t = 0;
};
}
