
.syntax unified
.cpu cortex-m4
.thumb

.section .text.reset_handler
    .global Reset_Handler
    .type Reset_Handler, %function
    Reset_Handler:
        ldr sp, =_estack
        bl  InitializeDataSegment
        bl  InitializeBssSegment
        bl  __libc_init_array   /* Calls static constructors */
        bl  main
        loop:                   /* Infinite loop in case main returns */
            b   loop
    .size Reset_Handler, . - Reset_Handler

.section .text.empty_isr
    .global __empty_isr
    .type __empty_isr, %function
    __empty_isr:
        bx  lr
    .size __empty_isr, . - __empty_isr

.macro default_isr name
    .global \name
    .weak \name
    .thumb_set \name, __empty_isr
    .word \name
.endm

/* Section .isr_vector with vector table */
.section .isr_vector, "a", %progbits
    .global __isr_vector_table
    .type __isr_vector_table, %object
    __isr_vector_table:
        .word _estack       /* The first word must point to the end of the stack */
        .word Reset_Handler /* The second word is the address of the reset handler */
        default_isr NMI_Handler
        default_isr HardFault_Handler
        default_isr MemManage_Handler
        default_isr BusFault_Handler
        default_isr UsageFault_Handler
        .word 0
        .word 0
        .word 0
        .word 0
        default_isr SVC_Handler
        default_isr DebugMon_Handler
        .word 0
        default_isr PendSV_Handler
        default_isr SysTick_Handler
        /* External Interrupts */
        default_isr WWDG_IRQHandler
        default_isr PVD_IRQHandler
        default_isr TAMP_STAMP_IRQHandler
        default_isr RTC_WKUP_IRQHandler
        default_isr FLASH_IRQHandler
        default_isr RCC_IRQHandler
        default_isr EXTI0_IRQHandler
        default_isr EXTI1_IRQHandler
        default_isr EXTI2_IRQHandler
        default_isr EXTI3_IRQHandler
        default_isr EXTI4_IRQHandler
        default_isr DMA1_Stream0_IRQHandler
        default_isr DMA1_Stream1_IRQHandler
        default_isr DMA1_Stream2_IRQHandler
        default_isr DMA1_Stream3_IRQHandler
        default_isr DMA1_Stream4_IRQHandler
        default_isr DMA1_Stream5_IRQHandler
        default_isr DMA1_Stream6_IRQHandler
        default_isr ADC_IRQHandler
        .word 0
        .word 0
        .word 0
        .word 0
        default_isr EXTI9_5_IRQHandler
        default_isr TIM1_BRK_TIM9_IRQHandler
        default_isr TIM1_UP_TIM10_IRQHandler
        default_isr TIM1_TRG_COM_TIM11_IRQHandler
        default_isr TIM1_CC_IRQHandler
        default_isr TIM2_IRQHandler
        default_isr TIM3_IRQHandler
        default_isr TIM4_IRQHandler
        default_isr I2C1_EV_IRQHandler
        default_isr I2C1_ER_IRQHandler
        default_isr I2C2_EV_IRQHandler
        default_isr I2C2_ER_IRQHandler
        default_isr SPI1_IRQHandler
        default_isr SPI2_IRQHandler
        default_isr USART1_IRQHandler
        default_isr USART2_IRQHandler
        .word 0
        default_isr EXTI15_10_IRQHandler
        default_isr RTC_Alarm_IRQHandler
        default_isr OTG_FS_WKUP_IRQHandler
        .word 0
        .word 0
        .word 0
        .word 0
        default_isr DMA1_Stream7_IRQHandler
        .word 0
        default_isr SDIO_IRQHandler
        default_isr TIM5_IRQHandler
        default_isr SPI3_IRQHandler
        .word 0
        .word 0
        .word 0
        .word 0
        default_isr DMA2_Stream0_IRQHandler
        default_isr DMA2_Stream1_IRQHandler
        default_isr DMA2_Stream2_IRQHandler
        default_isr DMA2_Stream3_IRQHandler
        default_isr DMA2_Stream4_IRQHandler
        .word 0
        .word 0
        .word 0
        .word 0
        .word 0
        .word 0
        default_isr OTG_FS_IRQHandler
        default_isr DMA2_Stream5_IRQHandler
        default_isr DMA2_Stream6_IRQHandler
        default_isr DMA2_Stream7_IRQHandler
        default_isr USART6_IRQHandler
        default_isr I2C3_EV_IRQHandler
        default_isr I2C3_ER_IRQHandler
        .word 0
        .word 0
        .word 0
        .word 0
        .word 0
        .word 0
        .word 0
        default_isr FPU_IRQHandler
        .word 0
        .word 0
        default_isr SPI4_IRQHandler
        default_isr SPI5_IRQHandler
    .size __isr_vector_table, . - __isr_vector_table
