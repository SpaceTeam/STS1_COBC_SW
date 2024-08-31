#include <Sts1CobcSw/Utility/Crc32Hardware.hpp>

#include <stm32f4xx.h>
#include <stm32f4xx_crc.h>
#include <stm32f4xx_dma.h>
#include <stm32f4xx_rcc.h>

#include <cstring>


namespace sts1cobcsw::utility
{
// --- Private globals ---

auto crcDmaStream = DMA2_Stream1;  // NOLINT(*no-int-to-ptr, *pro-type-cstyle-cast)
auto crcDmaTcif = DMA_FLAG_TCIF1;
auto crcDmaRcc = RCC_AHB1Periph_DMA2;


// --- Private function declarations ---

auto EnableCrcHardware() -> void;
auto EnableCrcDma() -> void;


// --- Public function definitions ---

//! @brief  Initialize the CRC DMA and the CRC peripheral itself.
auto InitializeCrc32Hardware() -> void
{
    EnableCrcDma();
    EnableCrcHardware();
}


//! @brief  Compute the CRC32 (MPEG-2) over a given data buffer in hardware with DMA.
//!
//! While DMA is used, the function currently polls the TCIF flag instead of using an interrupt.
//! Careful! Using DMA with the original buffer inverts endianness word-wise! E.g. buffer {0xDE,
//! 0xAD, 0xBE, 0xEF, 0xCA, 0xBB, 0xA5, 0xE3} is written (0xEFBEADDE, 0xE3A5BBCA), thus changing the
//! result!
//!
//! @param  data    The data buffer
//! @return The corresponding CRC32 checksum
auto ComputeCrc32(std::span<Byte const> data) -> std::uint32_t
{
    auto nTrailingBytes = data.size() % sizeof(std::uint32_t);

    DMA_Cmd(crcDmaStream, DISABLE);
    // The PAR (peripheral address register) requires the address as uint32_t
    // NOLINTNEXTLINE(*reinterpret-cast*)
    crcDmaStream->PAR = static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(data.data()));
    crcDmaStream->NDTR = static_cast<std::uint32_t>(data.size() - nTrailingBytes);
    CRC_ResetDR();
    DMA_Cmd(crcDmaStream, ENABLE);

    while(DMA_GetFlagStatus(crcDmaStream, crcDmaTcif) == RESET) {}
    DMA_ClearFlag(crcDmaStream, crcDmaTcif);

    if(nTrailingBytes > 0)
    {
        std::uint32_t trailingWord = 0;
        std::memcpy(&trailingWord, data.last(nTrailingBytes).data(), nTrailingBytes);
        CRC_CalcCRC(trailingWord);
    }
    return CRC_GetCRC();
}


//! @brief  Compute the CRC32 (MPEG-2) over a given data buffer in hardware with DMA.
//!
//! The const qualifier is omitted since the STM32 library function requires non-const uint32_t *
//!
//! @param  data    The data over which the CRC32 is calculated, requires uint32_t data
//! @return The corresponding CRC32 checksum
auto ComputeCrc32Blocking(std::span<std::uint32_t const> data) -> std::uint32_t
{
    CRC_ResetDR();
    // CRC_CalcBlockCRC() does not modify the data, so the const_cast is safe.
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
    return CRC_CalcBlockCRC(const_cast<std::uint32_t *>(data.data()),
                            static_cast<std::uint32_t>(data.size()));
}


// --- Private function definitions ---

//! @brief  Enable the CRC peripheral.
auto EnableCrcHardware() -> void
{
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_CRC, ENABLE);
}


//! @brief  Enable the CRC DMA.
//!
//! For more information see
//! https://www.st.com/resource/en/application_note/an4187-using-the-crc-peripheral-on-stm32-microcontrollers-stmicroelectronics.pdf.
auto EnableCrcDma() -> void
{
    RCC_AHB1PeriphClockCmd(crcDmaRcc, ENABLE);

    auto dmaInitStruct = DMA_InitTypeDef{};
    DMA_StructInit(&dmaInitStruct);

    dmaInitStruct.DMA_DIR = DMA_DIR_MemoryToMemory;
    // CRC->DR = CRC data register
    // NOLINTNEXTLINE(*reinterpret-cast*,*no-int-to-ptr,*pro-type-cstyle-cast)
    dmaInitStruct.DMA_Memory0BaseAddr = reinterpret_cast<std::uintptr_t>(&(CRC->DR));
    dmaInitStruct.DMA_PeripheralInc = DMA_PeripheralInc_Enable;
    dmaInitStruct.DMA_MemoryInc = DMA_MemoryInc_Disable;
    // Either write words, or a burst of 4 bytes
    dmaInitStruct.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    dmaInitStruct.DMA_MemoryDataSize = DMA_MemoryDataSize_Word;
    dmaInitStruct.DMA_Mode = DMA_Mode_Normal;
    dmaInitStruct.DMA_MemoryBurst = DMA_MemoryBurst_Single;
    dmaInitStruct.DMA_PeripheralBurst = DMA_PeripheralBurst_INC4;
    dmaInitStruct.DMA_FIFOMode = DMA_FIFOMode_Disable;
    // TODO: Check necessary DMA_InitStruct.DMA_Priority; default is low

    DMA_Init(crcDmaStream, &dmaInitStruct);
}
}
