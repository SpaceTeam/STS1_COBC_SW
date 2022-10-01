#include <Sts1CobcSw/Hal/IoNames.hpp>
#include <Sts1CobcSw/Periphery/Enums.hpp>

#include <rodos.h>

#include <span>
#include <tuple>

namespace sts1cobcsw::periphery
{

class EduUartInterface
{
    RODOS::HAL_UART mEduUart_ = HAL_UART(hal::eduUartIndex, hal::eduUartTxPin, hal::eduUartRxPin);
    bool mIsInitialized_ = false;

    /**
     * @brief Receive nBytes bytes over the EDU uart.
     *
     * @param recvVec The destination vector
     * @param nBytes The amount of bytes to receive
     *
     * @returns A relevant EDU error code
     */
    auto UartReceive(std::vector<uint8_t> & recvVec, size_t nBytes) -> EduErrorCode;

    /**
     * @brief Flush the EDU UART read buffer.
     *
     * This can be used to clear all buffer data after an error to request
     * a resend.
     */
    void FlushUartBuffer();

  public:
    /**
     * @brief Default constructor, also initializes the EDU UART.
     */
    EduUartInterface();

    /**
     * @brief Send a data packet over UART to the EDU.
     *
     * @param data The data to be sent
     */
    auto SendData(std::span<uint8_t> data) -> EduErrorCode;

    /**
     * @brief Send a CEP command to the EDU.
     *
     * @param cmd The command
     */
    void SendCommand(uint8_t cmd);

    /**
     * @brief Issues a command to execute a student program on the EDU.
     *
     * Execute Program (COBC <-> EDU):
     * -> [Command Header]
     * -> [Program ID]
     * -> [Queue ID]
     * -> [Timeout]
     * <- [N/ACK]
     * <- [N/ACK]
     *
     * The first N/ACK confirms a valid data packet,
     * the second N/ACK confirms that the program has been started.
     *
     * @param programId The student program ID
     * @param queueId The student program queue ID
     * @param timeout The available execution time for the student program
     *
     * @returns A relevant EduErrorCode
     */
    auto ExecuteProgram(uint16_t programId, uint16_t queueId, uint16_t timeout) -> EduErrorCode;

    /**
     * @brief Issues a command to get the student program status.
     *
     * Possible statuses:
     * No event: [1 byte: 0x00]
     * Program finished: [1 byte: 0x01][2 bytes: Program ID][2 bytes: Queue ID][1 byte: Exit Code]
     * Results ready: [1 byte: 0x02][2 bytes: Program ID][2 bytes: Queue ID]
     *
     * Get Status (COBC <-> EDU):
     * -> [Command Header]
     * <- [N/ACK]
     * <- [DATA]
     * <- [Status]
     * -> [N/ACK]
     *
     * @returns A tuple containing (Status Type, [Program ID], [Queue ID], [Exit Code], Error Code).
     * Values in square brackets are only valid if the relevant Status Type is returned.
     */
    auto GetStatus() -> std::tuple<EduStatusType, uint16_t, uint16_t, uint8_t, EduErrorCode>;

    /**
     * @brief Issued a command to update the EDU time.
     *
     * Update Time:
     * -> [Command Header]
     * -> [Timestamp]
     * <- [N/ACK]
     * <- [N/ACK]
     *
     * The first N/ACK confirms a valid data packet,
     * the second N/ACK confirms the time update.
     *
     * @param timestamp A unix timestamp
     *
     * @returns A relevant error code
     */
    auto UpdateTime(uint32_t timestamp) -> EduErrorCode;

    // TODO
    // auto StoreArchive() -> int32_t;
    // auto StopProgram() -> int32_t;
    // auto GetStatus() -> int32_t;
    // auto ReturnResult() -> int32_t;
    // auto UpdateTime() -> int32_t;
};
}