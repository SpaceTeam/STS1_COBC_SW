#include <Sts1CobcSw/Hal/IoNames.hpp>

#include <rodos.h>

#include <span>

namespace sts1cobcsw::periphery
{
class EduUartInterface
{
    RODOS::HAL_UART mEduUart_ = HAL_UART(hal::eduUartIndex, hal::eduUartTxPin, hal::eduUartRxPin);
    bool mIsInitialized_ = false;

  public:
    /**
     * @brief Default constructor, also initializes the EDU UART
     */
    EduUartInterface();

    /**
     * @brief Calculate CRC32 checksum for given data
     *
     * @param data The data for which the checksum is calculated
     * @param nBytes The length of the data, in bytes
     *
     * @returns The CRC32 checksum
     */
    auto Crc32(std::span<uint8_t> data, size_t nBytes) -> uint32_t;

    /**
     * @brief Send data over UART to the EDU
     * 
     * @param data The data to be sent
     */
    void SendData(std::span<uint8_t> data);
};
}