#include <Sts1CobcSw/Serial/Byte.hpp>
#include <Sts1CobcSw/Utility/Crc32.hpp>

#include <stm32f4xx_crc.h>
#include <stm32f4xx_dma.h>
#include <stm32f4xx_rcc.h>

#include <rodos_no_using_namespace.h>

#include <array>
#include <bit>
#include <climits>


namespace sts1cobcsw::utility
{
unsigned int const nBitsPerByte = CHAR_BIT;
std::uint32_t const initialCrc32Value = 0xFFFFFFFFU;

auto crcDma = DMA2_Stream1;        // NOLINT
auto crcDmaTcif = DMA_FLAG_TCIF1;  // NOLINT

constexpr auto crcTable = std::to_array<std::uint32_t>(
    {0x00000000, 0x04c11db7, 0x09823b6e, 0x0d4326d9, 0x130476dc, 0x17c56b6b, 0x1a864db2,
     0x1e475005, 0x2608edb8, 0x22c9f00f, 0x2f8ad6d6, 0x2b4bcb61, 0x350c9b64, 0x31cd86d3,
     0x3c8ea00a, 0x384fbdbd, 0x4c11db70, 0x48d0c6c7, 0x4593e01e, 0x4152fda9, 0x5f15adac,
     0x5bd4b01b, 0x569796c2, 0x52568b75, 0x6a1936c8, 0x6ed82b7f, 0x639b0da6, 0x675a1011,
     0x791d4014, 0x7ddc5da3, 0x709f7b7a, 0x745e66cd, 0x9823b6e0, 0x9ce2ab57, 0x91a18d8e,
     0x95609039, 0x8b27c03c, 0x8fe6dd8b, 0x82a5fb52, 0x8664e6e5, 0xbe2b5b58, 0xbaea46ef,
     0xb7a96036, 0xb3687d81, 0xad2f2d84, 0xa9ee3033, 0xa4ad16ea, 0xa06c0b5d, 0xd4326d90,
     0xd0f37027, 0xddb056fe, 0xd9714b49, 0xc7361b4c, 0xc3f706fb, 0xceb42022, 0xca753d95,
     0xf23a8028, 0xf6fb9d9f, 0xfbb8bb46, 0xff79a6f1, 0xe13ef6f4, 0xe5ffeb43, 0xe8bccd9a,
     0xec7dd02d, 0x34867077, 0x30476dc0, 0x3d044b19, 0x39c556ae, 0x278206ab, 0x23431b1c,
     0x2e003dc5, 0x2ac12072, 0x128e9dcf, 0x164f8078, 0x1b0ca6a1, 0x1fcdbb16, 0x018aeb13,
     0x054bf6a4, 0x0808d07d, 0x0cc9cdca, 0x7897ab07, 0x7c56b6b0, 0x71159069, 0x75d48dde,
     0x6b93dddb, 0x6f52c06c, 0x6211e6b5, 0x66d0fb02, 0x5e9f46bf, 0x5a5e5b08, 0x571d7dd1,
     0x53dc6066, 0x4d9b3063, 0x495a2dd4, 0x44190b0d, 0x40d816ba, 0xaca5c697, 0xa864db20,
     0xa527fdf9, 0xa1e6e04e, 0xbfa1b04b, 0xbb60adfc, 0xb6238b25, 0xb2e29692, 0x8aad2b2f,
     0x8e6c3698, 0x832f1041, 0x87ee0df6, 0x99a95df3, 0x9d684044, 0x902b669d, 0x94ea7b2a,
     0xe0b41de7, 0xe4750050, 0xe9362689, 0xedf73b3e, 0xf3b06b3b, 0xf771768c, 0xfa325055,
     0xfef34de2, 0xc6bcf05f, 0xc27dede8, 0xcf3ecb31, 0xcbffd686, 0xd5b88683, 0xd1799b34,
     0xdc3abded, 0xd8fba05a, 0x690ce0ee, 0x6dcdfd59, 0x608edb80, 0x644fc637, 0x7a089632,
     0x7ec98b85, 0x738aad5c, 0x774bb0eb, 0x4f040d56, 0x4bc510e1, 0x46863638, 0x42472b8f,
     0x5c007b8a, 0x58c1663d, 0x558240e4, 0x51435d53, 0x251d3b9e, 0x21dc2629, 0x2c9f00f0,
     0x285e1d47, 0x36194d42, 0x32d850f5, 0x3f9b762c, 0x3b5a6b9b, 0x0315d626, 0x07d4cb91,
     0x0a97ed48, 0x0e56f0ff, 0x1011a0fa, 0x14d0bd4d, 0x19939b94, 0x1d528623, 0xf12f560e,
     0xf5ee4bb9, 0xf8ad6d60, 0xfc6c70d7, 0xe22b20d2, 0xe6ea3d65, 0xeba91bbc, 0xef68060b,
     0xd727bbb6, 0xd3e6a601, 0xdea580d8, 0xda649d6f, 0xc423cd6a, 0xc0e2d0dd, 0xcda1f604,
     0xc960ebb3, 0xbd3e8d7e, 0xb9ff90c9, 0xb4bcb610, 0xb07daba7, 0xae3afba2, 0xaafbe615,
     0xa7b8c0cc, 0xa379dd7b, 0x9b3660c6, 0x9ff77d71, 0x92b45ba8, 0x9675461f, 0x8832161a,
     0x8cf30bad, 0x81b02d74, 0x857130c3, 0x5d8a9099, 0x594b8d2e, 0x5408abf7, 0x50c9b640,
     0x4e8ee645, 0x4a4ffbf2, 0x470cdd2b, 0x43cdc09c, 0x7b827d21, 0x7f436096, 0x7200464f,
     0x76c15bf8, 0x68860bfd, 0x6c47164a, 0x61043093, 0x65c52d24, 0x119b4be9, 0x155a565e,
     0x18197087, 0x1cd86d30, 0x029f3d35, 0x065e2082, 0x0b1d065b, 0x0fdc1bec, 0x3793a651,
     0x3352bbe6, 0x3e119d3f, 0x3ad08088, 0x2497d08d, 0x2056cd3a, 0x2d15ebe3, 0x29d4f654,
     0xc5a92679, 0xc1683bce, 0xcc2b1d17, 0xc8ea00a0, 0xd6ad50a5, 0xd26c4d12, 0xdf2f6bcb,
     0xdbee767c, 0xe3a1cbc1, 0xe760d676, 0xea23f0af, 0xeee2ed18, 0xf0a5bd1d, 0xf464a0aa,
     0xf9278673, 0xfde69bc4, 0x89b8fd09, 0x8d79e0be, 0x803ac667, 0x84fbdbd0, 0x9abc8bd5,
     0x9e7d9662, 0x933eb0bb, 0x97ffad0c, 0xafb010b1, 0xab710d06, 0xa6322bdf, 0xa2f33668,
     0xbcb4666d, 0xb8757bda, 0xb5365d03, 0xb1f740b4});

// Private function prototypes
// Interrupt handler not needed at the moment since we poll the TCIF
// extern "C"
// {
// void DMA2_Stream1_IRQHandler();
// }

auto EnableCrcHardware() -> void;
auto EnableCrcDma() -> void;

// Private function implementations

// Interrupt handler not needed at the moment since we poll the TCIF
// extern "C"
// {
// void DMA2_Stream1_IRQHandler()
// {
//     if(DMA_GetITStatus(DMA2_Stream1, DMA_IT_TCIF1))
//     {
//         DMA_ClearITPendingBit(DMA2_Stream1, DMA_IT_TCIF1);
//         NVIC_ClearPendingIRQ(DMA2_Stream1_IRQn);
//         // TODO: Make usable
//         RODOS::PRINTF("Interrupt CRC: %lx\n", CRC_GetCRC());
//     }
//     else
//     {
//         NVIC_ClearPendingIRQ(DMA2_Stream1_IRQn);
//     }
// }
// }

//! @brief  Enable the CRC peripheral.
auto EnableCrcHardware() -> void
{
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_CRC, ENABLE);
}


//! @brief  Enable the CRC DMA.
auto EnableCrcDma() -> void
{
    // Proposed: DMA 2 Stream 1

    // Enable DMA 2
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2, ENABLE);

    // Initialize
    // https://www.st.com/resource/en/application_note/an4187-using-the-crc-peripheral-on-stm32-microcontrollers-stmicroelectronics.pdf
    auto dmaInitStruct = DMA_InitTypeDef{};
    DMA_StructInit(&dmaInitStruct);

    // Check if this is needed, seen in an online example but not in the STM manual
    // DMA_DeInit(DMA2_Stream1);

    dmaInitStruct.DMA_DIR = DMA_DIR_MemoryToMemory;

    // Changed when there is an actual transfer, so keep default
    // dmaInitStruct.DMA_PeripheralBaseAddr = static_cast<uint32_t>(...);

    // M2M transfer -> Memory address is CRC data register address
    // NOLINTNEXTLINE(*reinterpret-cast*)
    dmaInitStruct.DMA_Memory0BaseAddr = reinterpret_cast<std::uintptr_t>(&(CRC->DR));
    dmaInitStruct.DMA_PeripheralInc = DMA_PeripheralInc_Enable;
    dmaInitStruct.DMA_MemoryInc = DMA_MemoryInc_Disable;

    // Changed when there is an actual transfer, so keep default
    // dmaInitStruct.DMA_BufferSize = static_cast<uint32_t>(...);

    // Either write words, or a burst of 4 bytes
    dmaInitStruct.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    dmaInitStruct.DMA_MemoryDataSize = DMA_MemoryDataSize_Word;
    dmaInitStruct.DMA_Mode = DMA_Mode_Normal;
    dmaInitStruct.DMA_MemoryBurst = DMA_MemoryBurst_Single;
    dmaInitStruct.DMA_PeripheralBurst = DMA_PeripheralBurst_INC4;
    dmaInitStruct.DMA_FIFOMode = DMA_FIFOMode_Disable;

    // TODO: Check necessary priority, default is low
    // DMA_InitStruct.DMA_Priority = DMA_Priority_something;

    DMA_Init(crcDma, &dmaInitStruct);

    // Enable DMA 2 Stream 1 Interrupt
    // Don't enable for now, since we poll the TCIF for now instead of using an interrupt
    // DMA_ITConfig(crcDma, DMA_IT_TC, DISABLE);
    // NVIC_EnableIRQ(DMA2_Stream1_IRQn);
}

// Public function implementations

//! @brief  Compute the CRC32 (MPEG-2) over a given data buffer in software.
//!
//! @param  data    The data buffer
//! @return The corresponding CRC32 checksum
auto ComputeCrc32Sw(std::span<Byte const> data) -> std::uint32_t
{
    // https://en.wikipedia.org/wiki/Cyclic_redundancy_check    -> What is CRC
    // https://reveng.sourceforge.io/crc-catalogue/all.htm      -> Description of MPEG-2
    // https://docs.rs/crc/3.0.0/src/crc/crc32.rs.html          -> Rust implementation (EDU)
    // https://gist.github.com/Miliox/b86b60b9755faf3bd7cf      -> C++ implementation
    // https://crccalc.com/                                     -> To check the implementation
    std::uint32_t crc32 = initialCrc32Value;
    for(auto const & element : data)
    {
        auto lookupIndex =
            // NOLINTNEXTLINE(*magic-numbers*)
            ((crc32 >> 24U) ^ std::to_integer<std::uint32_t>(element)) & 0xFFU;
        crc32 = (crc32 << nBitsPerByte) ^ crcTable[lookupIndex];
    }
    return crc32;
}


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
    static_assert(std::endian::native == std::endian::little);
    auto nTrailingBytes = data.size() % sizeof(std::uint32_t);

    DMA_Cmd(crcDma, DISABLE);

    // Set new data address
    // The PAR register requires the address as uint32_t so we allow reinterpret_cast and potential
    // shortening 64 bit to 32 bit warning
    // NOLINTNEXTLINE(*reinterpret-cast*, *shorten*)
    crcDma->PAR = reinterpret_cast<std::uintptr_t>(data.data());

    // Set new data length, reset CRC data register and start transfer
    auto dataLength = data.size() - nTrailingBytes;
    crcDma->NDTR = static_cast<std::uint32_t>(dataLength);
    CRC_ResetDR();
    DMA_Cmd(crcDma, ENABLE);

    // Temporary implementation: Poll TCIF, then write trailing bytes (or return)
    while(DMA_GetFlagStatus(crcDma, crcDmaTcif) == RESET)
    {
        // RODOS::Thread::yield();
    }
    DMA_ClearFlag(crcDma, crcDmaTcif);

    // Write remaining trailing bytes
    if(nTrailingBytes > 0)
    {
        std::uint32_t trailingWord = 0U;
        auto lastIndex = data.size() - 1;
        for(size_t i = 0; i < nTrailingBytes; i++)
        {
            trailingWord |= static_cast<std::uint32_t>(data[lastIndex - i])
                         << ((nTrailingBytes - i - 1) * CHAR_BIT);
        }
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
auto ComputeCrc32Blocking(std::span<std::uint32_t> data) -> std::uint32_t
{
    CRC_ResetDR();
    // BufferLength is uint32_t, so allow shortening size_t to uint32_t warning
    // NOLINTNEXTLINE(*shorten*)
    auto crc = CRC_CalcBlockCRC(data.data(), data.size());
    return crc;
}
}
