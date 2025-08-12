#include <Sts1CobcSw/Bootloader/RunFirmware.hpp>

#include <Sts1CobcSw/Bootloader/Fram.hpp>
#include <Sts1CobcSw/Bootloader/Print.hpp>
#include <Sts1CobcSw/Bootloader/stm32f411xe.h>
#ifdef ENABLE_DEBUG_PRINT
    #include <Sts1CobcSw/Bootloader/Leds.hpp>
    #include <Sts1CobcSw/Bootloader/UciUart.hpp>
#endif

#include <cstdint>


namespace sts1cobcsw
{
namespace
{
auto DisableConfigurableExceptions() -> void;
template<std::uint32_t address>
auto SetVectorTableOffsetRegister() -> void;
auto SetMainStackPointer(std::uint32_t stackPointer) -> void;
}


auto RunFirmware() -> void
{
    static constexpr auto primaryPartitionStartAddress = 0x0802'0000U;
    static constexpr auto metadataStartAddress = primaryPartitionStartAddress;
    static constexpr auto metadataSize = 0x200U;
    static constexpr auto firmwareStartAddress = metadataStartAddress + metadataSize;

    // NOLINTBEGIN(*reinterpret-cast, *no-int-to-ptr, *pointer-arithmetic)
    auto volatile * vectorTable = reinterpret_cast<std::uint32_t volatile *>(firmwareStartAddress);
    auto initialStackPointer = vectorTable[0];
    auto resetHandler = reinterpret_cast<void (*)()>(vectorTable[1]);
    // NOLINTEND(*reinterpret-cast, *no-int-to-ptr, *pointer-arithmetic)

    DEBUG_PRINT("Jumping to firmware...\n\n");
    sts1cobcsw::fram::Reset();
#ifdef ENABLE_DEBUG_PRINT
    sts1cobcsw::uciuart::Reset();
    sts1cobcsw::leds::TurnOff();
    sts1cobcsw::leds::Reset();
#endif

    DisableConfigurableExceptions();
    SetVectorTableOffsetRegister<firmwareStartAddress>();
    SetMainStackPointer(initialStackPointer);
    resetHandler();
    __builtin_unreachable();  // Tell the compiler that this function does indeed not return
}


namespace
{
auto DisableConfigurableExceptions() -> void
{
    asm volatile("CPSID i" : : : "memory");  // NOLINT(*no-assembler)
}


template<std::uint32_t address>
auto SetVectorTableOffsetRegister() -> void
{
    static constexpr auto alignment = 0x200U;
    static_assert((address & (alignment - 1U)) == 0U, "Address must be aligned to 512 bytes");
    static constexpr auto sramSize = 0x2'0000U;  // 128 KiB
    static constexpr auto addressIsInSram = SRAM_BASE <= address and address < SRAM_BASE + sramSize;
    static constexpr auto addressIsInFlash = FLASH_BASE <= address and address < FLASH_END;
    static_assert(addressIsInSram or addressIsInFlash, "Address must be in flash or SRAM");

    static constexpr auto vtorAddress = 0xE000'ED08U;  // See PM0214 Table 50
    // NOLINTNEXTLINE(*reinterpret-cast, *no-int-to-ptr)
    auto * volatile vtor = reinterpret_cast<std::uint32_t volatile *>(vtorAddress);
    *vtor = address;
}


auto SetMainStackPointer(std::uint32_t stackPointer) -> void
{
    asm volatile("MSR msp, %0" : : "r"(stackPointer) :);  // NOLINT(*no-assembler)
}
}
}
