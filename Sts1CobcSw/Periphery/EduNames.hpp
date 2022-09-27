#include <timemodel.h>

namespace sts1cobcsw::periphery
{
// CEP basic commands (see EDU PDD)
inline constexpr auto cmdAck = 0xd7;  /*! Acknowledging a data packet */
inline constexpr auto cmdNack = 0x27; /*! Not Acknowledging a (invalid) data packet */
inline constexpr auto cmdEof = 0x59;  /*! Transmission of multiple packets is complete */
inline constexpr auto cmdStop = 0xb4; /*! Transmission of multiple packets should be stopped */
inline constexpr auto cmdData = 0x8b; /*! Data packet format is used (not a command packet!) */

// CEP high level command headers (see EDU PDD)
inline constexpr auto storeArchive = 0x01;   /*! Transfer student programs from COBC to EDU */
inline constexpr auto executeProgram = 0x02; /*! Execute student program */
inline constexpr auto stopProgram = 0x03;    /*! Stop student program */
inline constexpr auto getStatus = 0x04;      /*! Get the student program status */
inline constexpr auto returnResults = 0x05;  /*! Request student program result */
inline constexpr auto updateTime = 0x06;     /*! Update EDU system time */

// Get Status result types
inline constexpr auto noEvent = 0x00;
inline constexpr auto programFinished = 0x01;
inline constexpr auto resultsReady = 0x02;
inline constexpr auto invalidStatus = 0xFF;

// Other transmission values

/** Max. length for a single round data field */
inline constexpr auto maxDataLen = 32768;

// TODO: check real timeout!
/** Max. time for the EDU to respond to a request */
inline constexpr auto eduTimeout = 5 * SECONDS;
/** Timeout used when flushing the UART receive buffer */
inline constexpr auto flushTimeout = 1 * MILLISECONDS;
/** UART flush garbage buffer size */
inline constexpr auto garbageBufSize = 128;

/** Max. amount of bytes for result of "get status" EDU command */
inline constexpr auto maxStatusBytes = 6;
/** Amount of bytes for the length field of a data command */
inline constexpr size_t lenBytes = 2;
/** Amount of bytes for a basic command or a high level command header */
inline constexpr size_t cmdBytes = 1;
/** Max. amount of send retries after receiving NACK */
inline constexpr auto maxNackRetries = 10;
}