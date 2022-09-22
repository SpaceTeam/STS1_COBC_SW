#include <Sts1CobcSw/Hal/IoNames.hpp>

#include <rodos.h>

#include <concepts>
#include <span>

namespace sts1cobcsw::periphery
{

class EduUartInterface
{
    // RODOS::HAL_UART mEduUart_ = HAL_UART(hal::eduUartIndex, hal::eduUartTxPin,
    // hal::eduUartRxPin);
    bool mIsInitialized_ = false;

  public:
    /**
     * @brief Default constructor, also initializes the EDU UART
     */
    EduUartInterface();

    /**
     * @brief Send a data packet over UART to the EDU
     *
     * @param data The data to be sent
     */
    void SendData(std::span<uint8_t> data);
    
    /**
     * @brief Send a CEP command to the EDU
     * 
     * @param cmd The command
     */
    void SendCommand(uint8_t cmd);

    /**
     * @brief Issues a command to execute a student program on the EDU
     * 
     * @param programId The student program ID
     * @param queueId The student program queue ID
     * @param timeout The available execution time for the student program
     * 
     * @returns 0 on success, -1 on failure
     */
    auto ExecuteProgram(uint16_t programId, uint16_t queueId, uint16_t timeout) -> int32_t;

    // TODO
    //auto StoreArchive() -> int32_t;
    //auto StopProgram() -> int32_t;
    //auto GetStatus() -> int32_t;
    //auto ReturnResult() -> int32_t;
    //auto UpdateTime() -> int32_t;
};
}