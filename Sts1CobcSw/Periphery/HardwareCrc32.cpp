#include <Sts1CobcSw/Periphery/HardwareCrc32.hpp>

#include <stm32f4xx_crc.h>
#include <stm32f4xx_dma.h>
#include <stm32f4xx_rcc.h>

#include <rodos_no_using_namespace.h>

#include <etl/bit.h>

#include <span>


namespace sts1cobcsw::periphery
{

constexpr auto bytesPerWord = 4U;
auto crcDma = DMA2_Stream1;


extern "C"
{
void DMA1_Stream1_IRQHandler();
}


auto EnableHardwareCrc() -> void
{
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_CRC, ENABLE);
}


auto EnableHardwareCrcAndDma() -> void
{
    // Proposed: DMA 2 Stream 1

    // Enable DMA 1
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2, ENABLE);

    // Initialize
    // https://www.st.com/resource/en/application_note/an4187-using-the-crc-peripheral-on-stm32-microcontrollers-stmicroelectronics.pdf
    DMA_InitTypeDef dmaInitStruct;
    DMA_StructInit(&dmaInitStruct);

    // Check if this is needed, seen in an online example but not in the STM manual
    // DMA_DeInit(DMA2_Stream1);
    dmaInitStruct.DMA_DIR = DMA_DIR_MemoryToMemory;

    // Changed when there is an actual transfer
    // dmaInitStruct.DMA_PeripheralBaseAddr = static_cast<uint32_t>(0U);

    dmaInitStruct.DMA_Memory0BaseAddr = reinterpret_cast<uintptr_t>(&(CRC->DR));
    dmaInitStruct.DMA_PeripheralInc = DMA_PeripheralInc_Enable;
    dmaInitStruct.DMA_MemoryInc = DMA_MemoryInc_Disable;

    // Changed when there is an actual transfer
    // dmaInitStruct.DMA_BufferSize = static_cast<uint32_t>(1U);

    dmaInitStruct.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Word;

    // TODO: Test this with byte and memory 4-burst instead of single and word
    dmaInitStruct.DMA_MemoryDataSize = DMA_MemoryDataSize_Word;
    dmaInitStruct.DMA_Mode = DMA_Mode_Normal;

    // TODO: Test this with byte and memory 4-burst instead of single and word
    dmaInitStruct.DMA_MemoryBurst = DMA_MemoryBurst_Single;
    dmaInitStruct.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
    dmaInitStruct.DMA_FIFOMode = DMA_FIFOMode_Disable;

    // TODO: Check necessary priority, default is low
    // DMA_InitStruct.DMA_Priority = DMA_Priority_something;

    DMA_Init(crcDma, &dmaInitStruct);
    DMA_ITConfig(crcDma, DMA_IT_TC, ENABLE);

    // Enable CRC Peripheral
    // TODO: Check if we do it once in the beginning, or with every CRC calculation
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_CRC, ENABLE);

    // Enable DMA 2 Stream 1 Interrupt
    NVIC_EnableIRQ(DMA2_Stream1_IRQn);
}

auto DmaCrc32(std::span<uint8_t> data) -> void
{
    // Format data byte buffer to words
    // If not divisible by 4 bytes, 1 word padding is needed
    auto trailingBytes = data.size() % bytesPerWord;
    auto padding = trailingBytes == 0 ? 0U : 1U;
    auto tmpBufferSize = data.size() / bytesPerWord + padding;
    uint32_t tmpBuffer[tmpBufferSize];

    // Careful: don't forget that trailing bytes can't be added like this
    for(size_t i = 0; i < tmpBufferSize - padding; i++)
    {
        uint32_t word = 0U;
        word |= (data[i * 4U] << CHAR_BIT * 3) | (data[i * 4U + 1] << CHAR_BIT * 2)
              | (data[i * 4U + 2] << CHAR_BIT) | (data[i * 4U + 3]);
        tmpBuffer[i] = word;
    }
    if(padding != 0)
    {
        tmpBuffer[tmpBufferSize - 1] = 0U;
        for(size_t i = 0; i < trailingBytes; i++)
        {
            // Last byte has no shift, second to last 1 byte, ...
            tmpBuffer[tmpBufferSize - 1] |= (data[data.size() - 1 - i] << (i * CHAR_BIT));
        }
        // RODOS::PRINTF("Last word: %lx\n", tmpBuffer[tmpBufferSize - 1]);
    }


    DMA_Cmd(crcDma, DISABLE);
    // Set new data address
    crcDma->PAR = reinterpret_cast<uintptr_t>(tmpBuffer);
    // Set new data length
    crcDma->NDTR = static_cast<uint32_t>(tmpBufferSize);
    CRC_ResetDR();
    DMA_Cmd(crcDma, ENABLE);
}

// Non-DMA HW CRC32
auto HardwareCrc32(std::span<uint32_t> data) -> uint32_t
{
    // TODO: CRC with DMA manual states that CRC clock should be enabled and disabled, is this
    // really necessary?
    // RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_CRC, ENABLE);
    CRC_ResetDR();
    auto crc = CRC_CalcBlockCRC(data.data(), data.size());
    // RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_CRC, DISABLE);
    return crc;
}

extern "C"
{
void DMA2_Stream1_IRQHandler()
{
    if(DMA_GetITStatus(DMA2_Stream1, DMA_IT_TCIF1))
    {
        DMA_ClearITPendingBit(DMA2_Stream1, DMA_IT_TCIF1);
        NVIC_ClearPendingIRQ(DMA2_Stream1_IRQn);
        // TODO: Make usable
        RODOS::PRINTF("Interrupt CRC: %lx\n", CRC_GetCRC());
    }
    else
    {
        NVIC_ClearPendingIRQ(DMA2_Stream1_IRQn);
    }
}
}
}  // namespace sts1cobcsw::periphery
