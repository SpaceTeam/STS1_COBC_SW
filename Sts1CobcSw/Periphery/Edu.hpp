#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/Periphery/EduNames.hpp>
#include <Sts1CobcSw/Periphery/EduStructs.hpp>
#include <Sts1CobcSw/Periphery/Enums.hpp>

#include <rodos_no_using_namespace.h>

#include <span>


namespace sts1cobcsw::periphery
{
using sts1cobcsw::serial::Byte;


class Edu
{
  public:
    Edu();

    auto SendData(std::span<Byte> data) -> EduErrorCode;
    void SendCommand(uint8_t cmd);
    auto ExecuteProgram(uint16_t programId, uint16_t queueId, uint16_t timeout) -> EduErrorCode;
    auto GetStatus() -> EduStatus;
    auto UpdateTime(uint32_t timestamp) -> EduErrorCode;
    auto StopProgram() -> EduErrorCode;
    auto ReturnResult(std::array<uint8_t, maxDataLen> & dest) -> ResultInfo;
    auto StoreArchive() -> int32_t;

  private:
    auto UartReceive(std::span<uint8_t> dest, std::size_t nBytes) -> EduErrorCode;
    void FlushUartBuffer();
    // RODOS::HAL_UART mEduUart_ = HAL_UART(hal::eduUartIndex, hal::eduUartTxPin,
    // hal::eduUartRxPin);
    RODOS::HAL_UART mEduUart_ =
        RODOS::HAL_UART(hal::uciUartIndex, hal::uciUartTxPin, hal::uciUartRxPin);
    bool mIsInitialized_ = false;
    bool mResultPending_ = false;
};
}